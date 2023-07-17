#include "../Scene.h"
#include "../Camera.h"
#include "../SceneRenderer.h"
#include "../Entity.h"
#include "../Component.h"
#include <Asset/AssetManager.h>
#include <Physics/PhysicsEngine.h>

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
		for (auto entity : idComponents)
		{
			UUID uuid = other->m_Registry.get<UUIDComponent>(entity).id;
			CreateEntity(entity, uuid);
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
		auto view = m_Registry.view<SpriteComponent>();
		
		auto cameras_view = m_Registry.view<CameraComponent>();
		for (auto& entity : cameras_view) {
			TRSComponent& trs = m_Registry.get<TRSComponent>(entity);
			CameraComponent& camera_component = m_Registry.get<CameraComponent>(entity);
			camera_component.camera->SetPosition(trs.translation);

			if (camera_component.camera->GetType() == CameraProjectionType::PROJECTION_3D) {
				Shared<Camera3D> camera_3D = ShareAs<Camera3D>(camera_component.camera);
				camera_3D->SetRotation(trs.rotation.y, trs.rotation.x);
			}

			camera_component.camera->CalculateMatrices();
		}

		m_Renderer->BeginScene(m_Camera);
		if (m_Camera != nullptr) {
			for (auto& entity : view) {
				TRSComponent trs = m_Registry.get<TRSComponent>(entity);
				SpriteComponent sprite_component = m_Registry.get<SpriteComponent>(entity);

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
	}

	Entity Scene::CreateEntity(const UUID& id)
	{
		Entity entity(this);
		entity.AddComponent<UUIDComponent>(id);
		entity.AddComponent<TagComponent>("Object");
		entity.AddComponent<TRSComponent>();
		entity.AddComponent<TransformComponent>();

		entity.GetComponent<TagComponent>().tag.reserve(256);

		m_Entities.emplace(id, entity);

		return entity;
	}

	Entity Scene::CreateEntity(entt::entity entity_id, const UUID& id /*= UUID()*/)
	{
		Entity entity(m_Registry.create(entity_id), this);
		entity.AddComponent<UUIDComponent>(id);
		entity.AddComponent<TagComponent>("Object");
		entity.AddComponent<TRSComponent>();
		entity.AddComponent<TransformComponent>();

		entity.GetComponent<TagComponent>().tag.reserve(256);

		m_Entities.emplace(id, entity);

		return entity;
	}

	void Scene::RemoveEntity(Entity entity)
	{
		m_Registry.destroy(entity);
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
		PhysicsEngine::Get()->LaunchRuntime(this);
	}

	void Scene::ShutdownRuntime()
	{
		PhysicsEngine::Get()->Reset();
	}

	void Scene::SetPhysicsSettings(const PhysicsSettings& settings)
	{
		m_PhysicsSettings = settings;
		PhysicsEngine::Get()->SetSettings(settings);
	}

	void Scene::Serialize(nlohmann::json& node)
	{
		node.emplace("Textures", nlohmann::json::object());
		node.emplace("Entities", nlohmann::json::object());

		nlohmann::json& texture_node = node["Textures"];

		auto tex_registry = AssetManager::Get()->GetTextureRegistry();
		for (auto& [id, texture] : *tex_registry) {
			texture_node.emplace(std::to_string(texture->GetId()), texture->GetSpecification().path.string().erase(0, std::filesystem::current_path().string().size() + 1));
		}

		nlohmann::json& entities_node = node["Entities"];

		for (auto& [id, entt] : m_Entities) {
			nlohmann::json entity_node;

			Entity entity(entt, this);

			entity.Serialize(entity_node);
			entities_node.emplace(std::to_string(id.Get()), entity_node);
		}

		node.emplace("Entities", entities_node);
	}

	void Scene::Deserialize(nlohmann::json& node)
	{
		m_Entities.clear();
		m_Registry.clear();

		nlohmann::json textures = node["Textures"];

		for (auto i = textures.begin(); i != textures.end(); i++) {
			std::string texture_path = std::filesystem::absolute(i.value().get<std::string>()).string();

			Omni::UUID id = AssetManager::Get()->LoadTexture(texture_path, std::stoull(i.key()));

			Shared<Image> texture = AssetManager::Get()->GetTexture(id);
			m_Renderer->AcquireTextureIndex(texture, SamplerFilteringMode::NEAREST);
		}

		nlohmann::json& entities_node = node["Entities"];
		for (auto i = entities_node.begin(); i != entities_node.end(); i++) {
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