#include "../ScriptEngine.h"

#include <Log/Logger.h>
#include <Core/UUID.h>
#include <Filesystem/Filesystem.h>
#include <Scene/Scene.h>
#include <Scene/Component.h>
#include <Scene/Entity.h>

#include "ScriptAPI.h"
#include "../ScriptClass.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/mono-gc.h>

#include <fstream>

namespace Omni {

	struct Internals {
		MonoDomain* mRootDomain = nullptr;
		MonoDomain* mAppDomain = nullptr;
		MonoAssembly* mCoreAssembly = nullptr;
		MonoAssembly* mAppAssembly = nullptr;
	} s_Internals;

	void ScriptEngine::Init()
	{
		s_Instance = new ScriptEngine;
	}

	void ScriptEngine::Shutdown()
	{
		delete s_Instance;
	}

	ScriptEngine::ScriptEngine()
		: m_CallbackArgsAllocator()
	{
		InitMonoInternals();
	}

	ScriptEngine::~ScriptEngine()
	{

	}

	void ScriptEngine::InitMonoInternals()
	{
		// assemblies path
		mono_set_assemblies_path("Resources/mono/lib");

		// Root domain
		mRootDomain = mono_jit_init("OmniScriptEngine");
		if (mRootDomain == nullptr)
			OMNIFORCE_CORE_INFO("[ScriptEngine]: failed to initialize root domain");
		else
			OMNIFORCE_CORE_INFO("[ScriptEngine]: initialized root domain");
		ResetData(); // initialize

		ScriptAPI::AddInternalCalls();
	}

	MonoImage* ScriptEngine::GetCoreImage()
	{
		return mono_assembly_get_image(mCoreAssembly);
	}

	MonoImage* ScriptEngine::GetAppImage()
	{
		return mono_assembly_get_image(mAppAssembly);
	}

	void ScriptEngine::LoadAssemblies()
	{
		
		mAppDomain = mono_domain_create_appdomain((char*)"ScriptEngine.dll", nullptr);
		mono_domain_set(mAppDomain, true);

		OMNIFORCE_ASSERT_TAGGED(mAppDomain != NULL, "Failed to create app domain");

		mCoreAssembly = (MonoAssembly*)ReadAssembly(FileSystem::GetWorkingDirectory().append("assets/scripts/assemblies/ScriptEngine.dll"));

		MonoImage* engine_core_image = mono_assembly_get_image(mCoreAssembly);
		mScriptBase = ScriptClass("Omni", "GameObject", true);
		OMNIFORCE_ASSERT_TAGGED(mScriptBase.IsValid(), "[ScriptEngine]: failed to extract base game object class from C# assembly");

		mInitMethodBase = mScriptBase.GetMethod("OnInit", 0);
		mUpdateMethodBase = mScriptBase.GetMethod("OnUpdate", 0);
		mBaseCtor = mScriptBase.GetMethod(".ctor", 1);

		// App assembly
		mAppAssembly = ReadAssembly(FileSystem::GetWorkingDirectory().append("assets/scripts/assemblies/gamescripts.dll"));

		MonoImage* app_image = mono_assembly_get_image(mAppAssembly);
		const MonoTableInfo* typedef_table = mono_image_get_table_info(app_image, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typedef_table);

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typedef_table, i, cols, MONO_TYPEDEF_SIZE);

			std::string type_namespace = mono_metadata_string_heap(app_image, cols[MONO_TYPEDEF_NAMESPACE]);
			std::string type_name = mono_metadata_string_heap(app_image, cols[MONO_TYPEDEF_NAME]);

			MonoClass* mono_class = mono_class_from_name(app_image, type_namespace.c_str(), type_name.c_str());

			if (!mono_class_is_subclass_of(mono_class, mScriptBase.Raw(), false))
				continue;

			ScriptClass script_class(type_namespace, type_name, false);

			mAvailableClassesList.emplace(script_class.GetFullName(), script_class);
		}

		mAssembliesLoaded = true;
	}

	void ScriptEngine::UnloadAssemblies()
	{
		mono_domain_set(mono_get_root_domain(), false);
		mono_domain_unload(mAppDomain);
		OMNIFORCE_CORE_TRACE("[ScriptEngine]: unloading assemblies...");
		mAppDomain = NULL;
		ResetData();
	}

	void ScriptEngine::ReloadAssemblies()
	{
		UnloadAssemblies();
		LoadAssemblies();
	}

	void ScriptEngine::OnUpdate()
	{
		for (auto& callback_info : m_PendingCallbacks) {
			Shared<RuntimeScriptInstance> object = callback_info.entity.GetComponent<ScriptComponent>().script_object;
			object->InvokeMethod(callback_info.method, callback_info.args.data());
		}

		m_PendingCallbacks.clear();
		m_CallbackArgsAllocator.Clear();

		if (m_GCTimer.ElapsedMilliseconds() > 100) {
			mono_gc_collect(mono_gc_max_generation());
			m_GCTimer.Reset();
		}
		
	}

	void ScriptEngine::DispatchExceptionHandling(MonoObject* exception)
	{
		MonoClass* exception_class = mono_object_get_class(exception);
		MonoClassField* message_field = mono_class_get_field_from_name(exception_class, "_message");

		MonoObject* to_string_failed_exc;
		MonoObject* value_object = mono_field_get_value_object(ScriptEngine::Get()->GetDomain(), message_field, exception);
		MonoString* mono_string = mono_object_to_string(value_object, &to_string_failed_exc);

		char* error_message = mono_string_to_utf8(mono_string);
		OMNIFORCE_CORE_ERROR("[Scripts]: {}", error_message);
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

			if (mAvailableClassesList.find(script_component.class_name) == mAvailableClassesList.end()) {
				OMNIFORCE_CLIENT_ERROR("No script class with name \"{}\" was found", script_component.class_name);
				entity.RemoveComponent<ScriptComponent>();
				continue;
			}

			script_component.script_object = mAvailableClassesList[script_component.class_name].AllocateObject(uuid_component.id);
		}

		auto group = registry->group<RigidBodyComponent, ScriptComponent>();
		m_PendingCallbacks.reserve(group.size());
	}

	void ScriptEngine::ShutdownRuntime()
	{
		entt::registry* registry = m_Context->GetRegistry();
		auto view = registry->view<ScriptComponent>();

		for (entt::entity e : view) {
			Entity entity(e, m_Context);
			ScriptComponent& script_component = entity.GetComponent<ScriptComponent>();
			script_component.script_object.reset();
		}

		mono_gc_collect(mono_gc_max_generation());

		mOnInitMethods.clear();
		mOnUpdateMethods.clear();


		m_Context = nullptr;
	}

	MonoAssembly* ScriptEngine::ReadAssembly(std::filesystem::path path)
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

	void ScriptEngine::ResetData()
	{
		mAvailableClassesList.clear();
		mOnInitMethods.clear();
		mOnUpdateMethods.clear();
		mInitMethodBase = nullptr;
		mUpdateMethodBase = nullptr;
		mAssembliesLoaded = false;
	}

}