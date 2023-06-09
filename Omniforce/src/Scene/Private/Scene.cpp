#include "../Scene.h"
#include "../Camera.h"
#include "../SceneRenderer.h"
#include "../Entity.h"
#include "../Component.h"
#include <Asset/AssetManager.h>

#include <nlohmann/json.hpp>

namespace Omni {

	Scene::Scene(SceneType type)
		: m_Type(type)
	{
		SceneRendererSpecification renderer_spec = {};
		renderer_spec.anisotropic_filtering = 16;

		m_Renderer = SceneRenderer::Create(renderer_spec);

		if (type == SceneType::SCENE_TYPE_3D) {
			m_Camera = std::make_shared<Camera3D>();

			auto camera_3D = ShareAs<Camera3D>(m_Camera);
			camera_3D->SetProjection(glm::radians(90.0f), 16.0 / 9.0, 0.0f, 100.0f);
			camera_3D->Move({ 0.0f, 0.0f, -50.0f });
		}
		else if (type == SceneType::SCENE_TYPE_2D) {
			m_Camera = std::make_shared<Camera2D>();
			auto camera_2D = ShareAs<Camera3D>(m_Camera);
			float32 aspect_ratio = 16.0f / 9.0f;
			camera_2D->SetProjection(-aspect_ratio, aspect_ratio, -1.0f, 1.0f);
		}

		Entity entity = CreateEntity();
		entity.AddComponent<CameraComponent>(m_Camera, true);
		entity.GetComponent<TagComponent>().tag = "Main camera";
	}

	void Scene::Destroy()
	{
		m_Renderer->Destroy();
	}

	void Scene::OnUpdate(float ts /*= 60.0f*/)
	{
		m_Renderer->BeginScene(m_Camera);

		m_Registry.sort<SpriteComponent>([](const auto& lhs, const auto& rhs) {
			return lhs.layer < rhs.layer;
		});
		auto view = m_Registry.view<SpriteComponent>();
		
		for (auto& entity : view) {
			TRSComponent trs = m_Registry.get<TRSComponent>(entity);
			SpriteComponent sprite_component = m_Registry.get<SpriteComponent>(entity);

			Sprite sprite;
			sprite.texture_id = m_Renderer->GetTextureIndex(sprite_component.texture);
			sprite.rotation = glm::radians(-trs.rotation.z);
			sprite.position = { trs.translation.x * sprite_component.aspect_ratio, trs.translation.y, trs.translation.z };
			sprite.size = { trs.scale.x * sprite_component.aspect_ratio, trs.scale.y };
			sprite.color_tint = sprite_component.color;

			m_Renderer->RenderSprite(sprite);
		}

		m_Renderer->EndScene();
	}

	Entity Scene::CreateEntity(const UUID& id)
	{
		Entity entity(this);
		entity.AddComponent<UUIDComponent>(id);
		entity.AddComponent<TagComponent>("Object");
		entity.AddComponent<TRSComponent>();
		entity.AddComponent<TransformComponent>();

		entity.GetComponent<TagComponent>().tag.reserve(256);

		m_Entities.push_back(entity);

		return entity;
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

		for (auto& entity : m_Entities) {
			uint64 tag = entity.GetComponent<UUIDComponent>().id.Get();
			nlohmann::json entity_node;

			entity.Serialize(entity_node);

			entities_node.emplace(std::to_string(tag), entity_node);

		}

		node.emplace("Entities", entities_node);
	}

	void Scene::Deserialize(nlohmann::json& node)
	{
		m_Entities.clear();
		m_Registry.clear();

		nlohmann::json textures = node["Textures"];

		for (auto i = textures.begin(); i != textures.end(); i++) {
			std::string texture_path = std::filesystem::current_path().string() + "/" + i.value().get<std::string>();

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