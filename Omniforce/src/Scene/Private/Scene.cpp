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
	}

	void Scene::Destroy()
	{
		m_Renderer->Destroy();
	}

	void Scene::OnUpdate(float ts /*= 60.0f*/)
	{
		
	}

	Entity Scene::CreateEntity(const std::string& name /*= "Entity"*/)
	{
		Entity entity(this);
		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<TRSComponent>();

		m_Entities.push_back(entity);

		entity.GetComponent<TagComponent>().tag.reserve(256);

		return entity;
	}

}