#include "../ScriptEngine.h"

#include <Log/Logger.h>
#include <Core/UUID.h>
#include <Filesystem/Filesystem.h>
#include <Scene/Scene.h>
#include <Scene/Component.h>
#include <Scene/Entity.h>

#include "ScriptAPI.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/mono-gc.h>

#include <fstream>

namespace Omni {

	struct Internals {
		MonoDomain* root_romain = nullptr;
		MonoDomain* app_domain = nullptr;
		MonoAssembly* core_assembly = nullptr;
		MonoAssembly* app_assembly = nullptr;
	} s_Internals;

	struct ScriptEngineData {
		MonoMethod* on_init_method;
		MonoMethod* on_update_method;
		MonoMethod* base_entity_class_ctor;
		bool has_assemblies = false;
		robin_hood::unordered_map<std::string, MonoClass*> available_classes_list;
		robin_hood::unordered_map<UUID, MonoMethod*> init_methods;
		robin_hood::unordered_map<UUID, MonoMethod*> update_methods;

		void Reset() {
			available_classes_list.clear();
			init_methods.clear();
			update_methods.clear();
			on_init_method = nullptr;
			on_update_method = nullptr;
			has_assemblies = false;
		}
	} s_ScriptEngineData;

	void ScriptEngine::Init()
	{
		s_Instance = new ScriptEngine;
	}

	void ScriptEngine::Shutdown()
	{
		delete s_Instance;
	}

	ScriptEngine::ScriptEngine()
	{
		InitMonoInternals();
	}

	ScriptEngine::~ScriptEngine()
	{

	}

	void ScriptEngine::InitMonoInternals()
	{
		// assemblies path
		mono_set_assemblies_path("resources/mono/lib");

		// Root domain
		s_Internals.root_romain = mono_jit_init("OmniScriptEngine");
		if (s_Internals.root_romain == nullptr)
			OMNIFORCE_CORE_INFO("[ScriptEngine]: failed to initialize root domain");
		else
			OMNIFORCE_CORE_INFO("[ScriptEngine]: initialized root domain");
		s_ScriptEngineData.Reset(); // initialize

		ScriptAPI::AddInternalCalls();
	}

	void ScriptEngine::LoadAssemblies()
	{
		s_Internals.app_domain = mono_domain_create_appdomain((char*)"ScriptEngine.dll", nullptr);
		mono_domain_set(s_Internals.app_domain, true);

		OMNIFORCE_ASSERT_TAGGED(s_Internals.app_domain != NULL, "Failed to create app domain");

		// Engine core assembly
		// TODO: compile C# engine core only in release mode
		if (OMNIFORCE_BUILD_CONFIG == OMNIFORCE_DEBUG_CONFIG) {
			s_Internals.core_assembly = (MonoAssembly*)ReadAssembly("resources/scripting/bin/Debug/ScriptEngine.dll");
		}
		else if (OMNIFORCE_BUILD_CONFIG == OMNIFORCE_RELEASE_CONFIG) {
			s_Internals.core_assembly = (MonoAssembly*)ReadAssembly("resources/scripting/bin/Release/ScriptEngine.dll");
		}

		MonoImage* engine_core_image = mono_assembly_get_image(s_Internals.core_assembly);
		MonoClass* base_entity_class = mono_class_from_name(engine_core_image, "Omni", "GameObject");
		OMNIFORCE_ASSERT_TAGGED(base_entity_class != NULL, "[ScriptEngine]: failed extract to base game object class from C# assembly");

		s_ScriptEngineData.on_init_method = mono_class_get_method_from_name(base_entity_class, "OnInit", 0);
		s_ScriptEngineData.on_update_method = mono_class_get_method_from_name(base_entity_class, "OnUpdate", 0);
		s_ScriptEngineData.base_entity_class_ctor = mono_class_get_method_from_name(base_entity_class, ".ctor", 1);

		// App assembly
		s_Internals.app_assembly = (MonoAssembly*)ReadAssembly(FileSystem::GetWorkingDirectory().append("assets/scripts/bin/gamescripts.dll"));

		MonoImage* app_image = mono_assembly_get_image(s_Internals.app_assembly);
		const MonoTableInfo* typedef_table = mono_image_get_table_info(app_image, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typedef_table);

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typedef_table, i, cols, MONO_TYPEDEF_SIZE);

			std::string type_namespace = mono_metadata_string_heap(app_image, cols[MONO_TYPEDEF_NAMESPACE]);
			std::string type_name = mono_metadata_string_heap(app_image, cols[MONO_TYPEDEF_NAME]);

			MonoClass* mono_class = mono_class_from_name(app_image, type_namespace.c_str(), type_name.c_str());

			if (!mono_class_is_subclass_of(mono_class, base_entity_class, false))
				continue;

			if (type_namespace.length())
				type_namespace.push_back('.');

			s_ScriptEngineData.available_classes_list.emplace(fmt::format("{}{}", type_namespace, type_name), mono_class);
		}

		s_ScriptEngineData.has_assemblies = true;
	}

	void ScriptEngine::UnloadAssemblies()
	{
		mono_domain_set(mono_get_root_domain(), false);
		mono_domain_unload(s_Internals.app_domain);
		OMNIFORCE_CORE_TRACE("[ScriptEngine]: unloading assemblies...");
		s_Internals.app_domain = NULL;
		s_ScriptEngineData.Reset();
	}

	void ScriptEngine::ReloadAssemblies()
	{
		UnloadAssemblies();
		LoadAssemblies();
	}

	bool ScriptEngine::HasAssemblies() const
	{
		return s_ScriptEngineData.has_assemblies;
	}

	void ScriptEngine::CallOnInit(Entity e)
	{
		UUIDComponent& uuid_component = e.GetComponent<UUIDComponent>();
		ScriptComponent& script_component = e.GetComponent<ScriptComponent>();
		UUID uuid = uuid_component.id;

		MonoObject* exc;
		mono_runtime_invoke(
			s_ScriptEngineData.init_methods[uuid],
			script_component.runtime_managed_object,
			nullptr,
			&exc
		);
	}

	void ScriptEngine::CallOnUpdate(Entity e)
	{
		UUIDComponent& uuid_component = e.GetComponent<UUIDComponent>();
		ScriptComponent& script_component = e.GetComponent<ScriptComponent>();
		UUID uuid = uuid_component.id;

		MonoObject* exc;
		mono_runtime_invoke(
			s_ScriptEngineData.update_methods[uuid],
			script_component.runtime_managed_object,
			nullptr,
			&exc
		);
	}

	void ScriptEngine::LaunchRuntime(Scene* context)
	{
		m_Context = context;

		entt::registry* registry = context->GetRegistry();
		auto view = registry->view<ScriptComponent>();

		for (entt::entity e : view) {
			Entity entity(e, context);
			ScriptComponent& script_component = entity.GetComponent<ScriptComponent>();
			UUIDComponent& uuid_component = entity.GetComponent<UUIDComponent>();

			if (s_ScriptEngineData.available_classes_list.find(script_component.class_name) == s_ScriptEngineData.available_classes_list.end()) {
				OMNIFORCE_CLIENT_ERROR("No script class with name \"{}\" was found", script_component.class_name);
				entity.RemoveComponent<ScriptComponent>();
				continue;
			}

			script_component.runtime_managed_object = mono_object_new(s_Internals.app_domain, s_ScriptEngineData.available_classes_list[script_component.class_name]);

			// call base class constructor
			{
				void* parameter = &uuid_component.id;
				MonoObject* exception = nullptr;
				mono_runtime_invoke(s_ScriptEngineData.base_entity_class_ctor, script_component.runtime_managed_object, &parameter, &exception);
			}

			s_ScriptEngineData.init_methods.emplace(
				uuid_component.id, 
				mono_object_get_virtual_method((MonoObject*)script_component.runtime_managed_object, s_ScriptEngineData.on_init_method)
			);
			s_ScriptEngineData.update_methods.emplace(
				uuid_component.id,
				mono_object_get_virtual_method((MonoObject*)script_component.runtime_managed_object, s_ScriptEngineData.on_update_method)
			);
		}
	}

	void ScriptEngine::ShutdownRuntime()
	{
		mono_gc_collect(mono_gc_max_generation());
		entt::registry* registry = m_Context->GetRegistry();
		auto view = registry->view<ScriptComponent>();

		for (entt::entity e : view) {
			Entity entity(e, m_Context);
			ScriptComponent& script_component = entity.GetComponent<ScriptComponent>();
			script_component.runtime_managed_object = nullptr;
		}

		s_ScriptEngineData.init_methods.clear();
		s_ScriptEngineData.update_methods.clear();

		m_Context = nullptr;
	}

	void* ScriptEngine::ReadAssembly(std::filesystem::path path)
	{
		std::ifstream input_stream(path, std::ios::ate | std::ios::binary);

		std::streampos end = input_stream.tellg();
		input_stream.seekg(0, std::ios::beg);
		uint32_t assembly_size = end - input_stream.tellg();

		char* buffer = new char[assembly_size];
		input_stream.read(buffer, assembly_size);
		input_stream.close();

		MonoImageOpenStatus image_open_status;

		MonoImage* image = mono_image_open_from_data_full(buffer, assembly_size, 1, &image_open_status, 0);
		OMNIFORCE_ASSERT_TAGGED(image_open_status == MONO_IMAGE_OK, mono_image_strerror(image_open_status));

		delete[] buffer;

		MonoAssembly* mono_assembly = mono_assembly_load_from_full(image, path.filename().string().c_str(), &image_open_status, 0);
		OMNIFORCE_ASSERT_TAGGED(image_open_status == MONO_IMAGE_OK, mono_image_strerror(image_open_status));

		mono_image_close(image);

		OMNIFORCE_CORE_TRACE("[ScriptEngine]: loaded {}", path.filename().string());

		return mono_assembly;
	}

}