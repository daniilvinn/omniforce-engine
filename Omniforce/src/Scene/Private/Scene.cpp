#include "../Scene.h"
#include "../Camera.h"
#include "../SceneRenderer.h"
#include "../Entity.h"
#include "../Component.h"

namespace Omni {

	Scene::Scene(SceneType type)
		: m_Type(type)
	{
		SceneRendererSpecification renderer_spec = {};
		renderer_spec.anisotropic_filtering = 16;

		m_Renderer = SceneRenderer::Create(renderer_spec);

		m_Camera = std::make_shared<Camera3D>();

		auto camera_3D = ShareAs<Camera3D>(m_Camera);
		camera_3D->SetProjection(glm::radians(90.0f), 16.0 / 9.0, 0.0f, 100.0f);
		camera_3D->Move({ 0.0f, 0.0f, -50.0f });
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
			sprite.position = { trs.translation.x, trs.translation.y, trs.translation.z };
			sprite.size = { trs.scale.x, trs.scale.y };
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

		m_Entities.push_back(entity);

		entity.GetComponent<TagComponent>().tag.reserve(256);

		return entity;
	}

}