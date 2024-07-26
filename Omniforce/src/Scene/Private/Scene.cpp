#include "../Scene.h"
#include "../Camera.h"
#include "../SceneRenderer.h"
#include "../Entity.h"
#include "../Component.h"
#include "../Lights.h"
#include <Asset/AssetManager.h>
#include <Asset/OFRController.h>
#include <Physics/PhysicsEngine.h>
#include <Filesystem/Filesystem.h>
#include <Scripting/ScriptEngine.h>
#include <Threading/JobSystem.h>
#include <Core/Utils.h>

#include <nlohmann/json.hpp>

namespace Omni {

	template<typename Component>
	static void ExplicitComponentCopy(entt::registry& src_registry, entt::registry& dst_registry, robin_hood::unordered_map<UUID, entt::entity>& map) {
		auto components = src_registry.view<Component>();
		for (auto& src_entity : components)
		{
			entt::entity dst_entity = map.at(src_registry.get<UUIDComponent>(src_entity).id);
			auto& src_component = src_registry.get<Component>(src_entity);
			dst_registry.emplace_or_replace<Component>(dst_entity, src_component);
		}
	};

	Scene::Scene(SceneType type)
		: m_Type(type)
	{
		SceneRendererSpecification renderer_spec = {};
		renderer_spec.anisotropic_filtering = 16;

		m_Renderer = SceneRenderer::Create(renderer_spec);
	}

	Scene::Scene(Scene* other)
	{
		m_Registry.clear();

		m_Renderer = other->m_Renderer;
		m_Camera = other->m_Camera;

		auto idComponents = other->m_Registry.view<UUIDComponent>();
		for (auto entity = idComponents.rbegin(); entity != idComponents.rend(); entity++)
		{
			UUID uuid = other->m_Registry.get<UUIDComponent>(*entity).id;
			CreateEntity(*entity, uuid);
		}

		ExplicitComponentCopy<UUIDComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<TagComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<TRSComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<SpriteComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<CameraComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<RigidBodyComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<BoxColliderComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<SphereColliderComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<ScriptComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<HierarchyNodeComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<MeshComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<PointLightComponent>(other->m_Registry, m_Registry, other->m_Entities);

		m_PhysicsSettings = other->GetPhysicsSettings();
	}

	void Scene::Destroy()
	{
		m_Renderer->Destroy();
	}

	void Scene::OnUpdate(float32 step)
	{
		// Sort sprite components by their layer, so they and their depth are rendered correctly
		m_Registry.sort<SpriteComponent>([](const auto& lhs, const auto& rhs) {
			return lhs.layer < rhs.layer;
		});

		auto cameras_view = m_Registry.view<CameraComponent>();

		// Update camera components so view matches camera entity's position and rotation
		if (m_InRuntime) {
			for (auto& e : cameras_view) {
				Entity entity(e, this);

				TRSComponent world_transform = entity.GetWorldTransform();
				CameraComponent& camera_component = entity.GetComponent<CameraComponent>();

				camera_component.camera->SetPosition(world_transform.translation);

				if (camera_component.camera->GetType() == CameraProjectionType::PROJECTION_3D) {
					Shared<Camera3D> camera_3D = ShareAs<Camera3D>(camera_component.camera);
					glm::vec3 euler_angles = glm::degrees(glm::eulerAngles(world_transform.rotation));
					camera_3D->SetRotation(euler_angles.y, euler_angles.x);
				}

				camera_component.camera->CalculateMatrices();

				if(camera_component.primary)
					m_Camera = camera_component.camera;
			}
		}

		// Begin rendering
		m_Renderer->BeginScene(m_Camera);
		// If primary camera was not found, then it is nullptr and we cannot render
		if (m_Camera != nullptr) {
			m_Registry.view<SpriteComponent>().each([&, scene = this](auto e, auto& sprite_component) {
				Entity entity(e, this);
				TRSComponent trs_component = entity.GetWorldTransform();
				TagComponent& tag_component = entity.GetComponent<TagComponent>();

				glm::uvec2 packed_quat = {
					glm::packSnorm2x16({trs_component.rotation.x, trs_component.rotation.y}),
					glm::packSnorm2x16({trs_component.rotation.z, trs_component.rotation.w})
				};

				// Assemble sprite structure
				Sprite sprite;
				sprite.texture_id = m_Renderer->GetTextureIndex(sprite_component.texture);
				sprite.rotation = packed_quat;
				sprite.position = { trs_component.translation.x, trs_component.translation.y, trs_component.translation.z };
				sprite.size = { trs_component.scale.x * sprite_component.aspect_ratio, trs_component.scale.y };
				sprite.color_tint = sprite_component.color;

				// Add sprite to a render queue
				m_Renderer->RenderSprite(sprite);
			});

			// 3D
			m_Registry.view<MeshComponent>().each([&, asset_manager = AssetManager::Get(), scene = this](auto e, auto& mesh_data) {
				Entity entity(e, this);
				TRSComponent trs_component = entity.GetWorldTransform();

				DeviceRenderableObject renderable_object = {};
				renderable_object.trs.translation = trs_component.translation;
				renderable_object.trs.rotation = {
					glm::packSnorm2x16({trs_component.rotation.x, trs_component.rotation.y}),
					glm::packSnorm2x16({trs_component.rotation.z, trs_component.rotation.w})
				};
				renderable_object.trs.scale = trs_component.scale;
				renderable_object.render_data_index = m_Renderer->GetMeshIndex(mesh_data.mesh_handle);
				renderable_object.material_bda = m_Renderer->GetMaterialBDA(mesh_data.material_handle);
				m_Renderer->RenderObject(asset_manager->GetAsset<Material>(mesh_data.material_handle)->GetPipeline(), renderable_object);
			});

			// Lighiting
			m_Registry.view<PointLightComponent>().each([&, scene = this](auto e, auto& point_light) {
				Entity entity(e, this);
				TRSComponent trs_component = entity.GetWorldTransform();

				PointLight light_data = {};
				light_data.position = trs_component.translation;
				light_data.color = point_light.color;
				light_data.intensity = point_light.intensity;
				light_data.min_radius = point_light.min_radius;
				light_data.radius = point_light.radius;

				m_Renderer->AddPointLight(light_data);

			});

		}
		// End rendering and submit sprites to GPU
		m_Renderer->EndScene();

		// Simulate physics world and fetch results into a TRSComponent (if physics engine has no context, then we
		// are in editor mode and no simulation needed)
		PhysicsEngine* physics_engine = PhysicsEngine::Get();
		if (physics_engine->HasContext()) {
			physics_engine->Update();
			physics_engine->FetchResults();
		}

		// Update scripts (if script engine has no context, then we are in editor mode and no scripts update needed)
		ScriptEngine* script_engine = ScriptEngine::Get();
		if (script_engine->HasContext()) {
			script_engine->OnUpdate();

			auto script_component_view = m_Registry.view<ScriptComponent>();
			for (auto& e : script_component_view) {
				Entity entity(e, this);
				entity.GetComponent<ScriptComponent>().script_object->InvokeUpdate();
			}
		}
	}

	Entity Scene::CreateEntity(const UUID& id)
	{
		return CreateChildEntity(Entity(), id);
	}

	Entity Scene::CreateEntity(entt::entity entity_id, const UUID& id /*= UUID()*/)
	{
		Entity entity(m_Registry.create(entity_id), this);
		entity.AddComponent<UUIDComponent>(id);
		entity.AddComponent<TagComponent>("Object");
		entity.AddComponent<TRSComponent>();
		entity.AddComponent<HierarchyNodeComponent>().parent = {};

		entity.GetComponent<TagComponent>().tag.reserve(256);

		m_Entities.emplace(id, entity);

		return entity;
	}

	Entity Scene::CreateChildEntity(Entity parent, const UUID& id /*= UUID()*/)
	{
		Entity entity(this);
		entity.AddComponent<UUIDComponent>(id);
		entity.AddComponent<TagComponent>("Object");
		entity.AddComponent<TRSComponent>();
		entity.AddComponent<HierarchyNodeComponent>().parent = parent ? parent.GetID() : UUID(0);

		entity.GetComponent<TagComponent>().tag.reserve(256);

		if(parent)
			parent.GetComponent<HierarchyNodeComponent>().children.push_back(id);

		m_Entities.emplace(id, entity);


		return entity;
	}

	Entity Scene::GetEntity(std::string_view tag)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const TagComponent& tc = view.get<TagComponent>(entity);
			if (tc.tag == tag)
				return Entity( entity, this );
		}
		return {};
	}

	Entity Scene::GetEntity(UUID id)
	{
		return Entity(m_Entities[id], this);
	}

	void Scene::RemoveEntity(Entity entity)
	{
		UUIDComponent uuid_component = entity.GetComponent<UUIDComponent>();
		HierarchyNodeComponent& hierarchy_node_component = entity.GetComponent<HierarchyNodeComponent>();

		for (auto& child : hierarchy_node_component.children) {
			HierarchyNodeComponent& child_node_component = GetEntity(child).GetComponent<HierarchyNodeComponent>();
			child_node_component.parent = hierarchy_node_component.parent == 0 ? UUID(0) : hierarchy_node_component.parent;
		}
		if (hierarchy_node_component.parent.Valid()) {
			Entity parent = GetEntity(hierarchy_node_component.parent);
			HierarchyNodeComponent& parent_node_component = parent.GetComponent<HierarchyNodeComponent>();

			for (auto& child : hierarchy_node_component.children)
				parent_node_component.children.push_back(child);

			for (auto i = parent_node_component.children.begin(); i != parent_node_component.children.end(); i++) {
				if (*i == uuid_component.id) {
					parent_node_component.children.erase(i);
					break;
				}
			}
		}

		m_Entities.erase(entity.GetID());
		m_Registry.destroy(entity);
	}

	void Scene::RemoveEntityWithChildren(Entity entity)
	{
		UUIDComponent uuid_component = entity.GetComponent<UUIDComponent>();
		HierarchyNodeComponent& hierarchy_node_component = entity.GetComponent<HierarchyNodeComponent>();
		for (auto& child : hierarchy_node_component.children) {
			RemoveEntityWithChildren(GetEntity(child));
		}
		RemoveEntity(entity);
	}

	void Scene::LaunchRuntime()
	{
		m_InRuntime = true;;
		m_Camera = nullptr;
		auto view = m_Registry.view<CameraComponent>();
		for (auto& entity : view) {
			CameraComponent& camera_component = m_Registry.get<CameraComponent>(entity);
			if (camera_component.primary) {
				m_Camera = camera_component.camera;
				break;
			}
		}
		PhysicsEngine* physics_engine = PhysicsEngine::Get();
		physics_engine->LaunchRuntime(this);
		physics_engine->SetSettings(m_PhysicsSettings);

		ScriptEngine* script_engine = ScriptEngine::Get();
		script_engine->LaunchRuntime(this);
		auto script_component_view = m_Registry.view<ScriptComponent>();
		for (auto& e : script_component_view) {
			Entity entity(e, this);
			entity.GetComponent<ScriptComponent>().script_object->InvokeInit();
		}
	}

	void Scene::ShutdownRuntime()
	{
		m_InRuntime = false;
		PhysicsEngine::Get()->Reset();
		ScriptEngine::Get()->ShutdownRuntime();
	}

	fvec3 Scene::TraverseSceneHierarchy(Entity node, TRSComponent origin)
	{
		return { 0, 0, 0 };
	}

	void Scene::SetPhysicsSettings(const PhysicsSettings& settings)
	{
		m_PhysicsSettings = settings;
		PhysicsEngine::Get()->SetSettings(settings);
	}

	void Scene::Serialize(nlohmann::json& node)
	{
		node.emplace("Textures", nlohmann::json::object());
		node.emplace("GameObjects", nlohmann::json::object());

		nlohmann::json& texture_node = node["Textures"];

		auto tex_registry = *AssetManager::Get()->GetAssetRegistry();
		for (auto& [id, texture] : tex_registry) {
			auto image = ShareAs<Image>(texture);
			texture_node.emplace(std::to_string(texture->Handle), image->GetSpecification().path.string());
		}

		nlohmann::json& entities_node = node["GameObjects"];

		auto view = m_Registry.view<UUIDComponent>();

		for (auto& e : view) {
			Entity entity(e, this);

			if (!entity.GetComponent<HierarchyNodeComponent>().parent.Get()) {
				nlohmann::json entity_node;
				entity.Serialize(entity_node);
				entities_node.emplace(std::to_string(entity.GetComponent<UUIDComponent>().id), entity_node);
			}
		}

		node.emplace("GameObjects", entities_node);
	}

	void Scene::Deserialize(nlohmann::json& node)
	{
		m_Entities.clear();
		m_Registry.clear();

		nlohmann::json textures = node["Textures"];

		std::shared_mutex renderer_mtx;
		auto executor = JobSystem::GetExecutor();

		tf::Taskflow taskflow;
		for (auto i : textures.items()) {
			taskflow.emplace([&, i]() {
				std::string texture_path = i.value().get<std::string>();

				OFRController ofr_controller(FileSystem::GetWorkingDirectory().append(texture_path));
				auto data = ofr_controller.ExtractSubresources();

				AssetFileHeader header = ofr_controller.ExtractHeader();
				auto metadata = ofr_controller.ExtractMetadata();
				uint32 num_mip_levels = 0;
				for (auto& i : metadata) {
					if (!i.decompressed_size)
						break;
					num_mip_levels++;
				}

				uint32 image_width = (uint32)(header.additional_data & UINT32_MAX);
				uint32 image_height = (uint32)(header.additional_data >> 32);

				ImageSpecification image_spec = ImageSpecification::Default();
				image_spec.extent = { image_width, image_height, 1 };
				image_spec.format = ImageFormat::BC7;
				image_spec.mip_levels = num_mip_levels;
				image_spec.path = texture_path;
				image_spec.pixels = data;
				image_spec.format = ImageFormat::BC7;
				image_spec.mip_levels = Utils::ComputeNumMipLevelsBC7(image_width, image_height) + 1;

				Shared<AssetBase> image = Image::Create(image_spec, 0);

				AssetHandle id = AssetManager::Get()->RegisterAsset(image, std::stoull(i.key()));

				Shared<Image> texture = AssetManager::Get()->GetAsset<Image>(id);

				renderer_mtx.lock();
				m_Renderer->AcquireResourceIndex(texture, SamplerFilteringMode::NEAREST);
				renderer_mtx.unlock();
			});
		}
		executor->run(taskflow).wait();

		nlohmann::json& entities_node = node["GameObjects"];
		for (auto i : entities_node.items()) {
			Entity entity = CreateEntity(std::stoull(i.key()));
			entity.Deserialize(i.value());
		}

		// look up primary camera
		auto camera_component_view = m_Registry.view<CameraComponent>();
		for (auto& camera_entity : camera_component_view) {
			Entity entity(camera_entity, this);
			if (entity.GetComponent<CameraComponent>().primary) {
				m_Camera = entity.GetComponent<CameraComponent>().camera;
				break;
			}
		}
	}
}