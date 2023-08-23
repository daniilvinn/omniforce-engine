#include "../Scene.h"
#include "../Camera.h"
#include "../SceneRenderer.h"
#include "../Entity.h"
#include "../Component.h"
#include <Asset/AssetManager.h>
#include <Physics/PhysicsEngine.h>
#include <Filesystem/Filesystem.h>
#include <Scripting/ScriptEngine.h>

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
		ExplicitComponentCopy<TransformComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<TRSComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<SpriteComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<CameraComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<RigidBody2DComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<BoxColliderComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<SphereColliderComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<ScriptComponent>(other->m_Registry, m_Registry, other->m_Entities);
		ExplicitComponentCopy<HierarchyNodeComponent>(other->m_Registry, m_Registry, other->m_Entities);

		m_PhysicsSettings = other->GetPhysicsSettings();
	}

	void Scene::Destroy()
	{
		m_Renderer->Destroy();
	}

	void Scene::OnUpdate(float32 step)
	{
		m_Registry.sort<SpriteComponent>([](const auto& lhs, const auto& rhs) {
			return lhs.layer < rhs.layer;
		});

		auto sprite_view = m_Registry.view<SpriteComponent>();
		auto cameras_view = m_Registry.view<CameraComponent>();

		for (auto& e : cameras_view) {
			Entity entity(e, this);

			TRSComponent world_transform = entity.GetWorldTransform();
			CameraComponent& camera_component = entity.GetComponent<CameraComponent>();

			camera_component.camera->SetPosition(world_transform.translation);

			if (camera_component.camera->GetType() == CameraProjectionType::PROJECTION_3D) {
				Shared<Camera3D> camera_3D = ShareAs<Camera3D>(camera_component.camera);
				camera_3D->SetRotation(world_transform.rotation.y, world_transform.rotation.x);
			}

			camera_component.camera->CalculateMatrices();
		}

		m_Renderer->BeginScene(m_Camera);
		if (m_Camera != nullptr) {
			for (auto& e : sprite_view) {
				Entity entity(e, this);

				TRSComponent trs = entity.GetWorldTransform();
				SpriteComponent sprite_component = entity.GetComponent<SpriteComponent>();

				Sprite sprite;
				sprite.texture_id = m_Renderer->GetTextureIndex(sprite_component.texture);
				sprite.rotation = glm::radians(trs.rotation.z);
				sprite.position = { trs.translation.x, trs.translation.y, trs.translation.z };
				sprite.size = { trs.scale.x * sprite_component.aspect_ratio, trs.scale.y };
				sprite.color_tint = sprite_component.color;

				m_Renderer->RenderSprite(sprite);
			}
		}
		m_Renderer->EndScene();

		PhysicsEngine* physics_engine = PhysicsEngine::Get();
		if (physics_engine->HasContext()) {
			physics_engine->Update();
			physics_engine->FetchResults();
		}

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
		entity.AddComponent<TransformComponent>();
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
		entity.AddComponent<TransformComponent>();
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

		auto tex_registry = AssetManager::Get()->GetTextureRegistry();
		for (auto& [id, texture] : *tex_registry) {
			texture_node.emplace(std::to_string(texture->GetId()), texture->GetSpecification().path.string().erase(0, FileSystem::GetWorkingDirectory().string().size()));
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

		for (auto i : textures.items()) {
			std::string texture_path = i.value().get<std::string>();

			UUID id = AssetManager::Get()->LoadTexture(FileSystem::GetWorkingDirectory().append(texture_path), std::stoull(i.key()));

			Shared<Image> texture = AssetManager::Get()->GetTexture(id);
			m_Renderer->AcquireTextureIndex(texture, SamplerFilteringMode::NEAREST);
		}

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