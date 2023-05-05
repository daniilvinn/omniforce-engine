#include "../Scene.h"
#include "../Camera.h"
#include "../SceneRenderer.h"

namespace Omni {

	Scene::Scene(SceneType type)
		: m_Type(type)
	{
		switch (m_Type)
		{
		case SceneType::SCENE_TYPE_2D:		m_Camera = new Camera2D; break;
		case SceneType::SCENE_TYPE_3D:		m_Camera = new Camera3D; break;
		default:							std::unreachable();
		}

		SceneRendererSpecification renderer_spec = {};
		renderer_spec.anisotropic_filtering = 16;

		m_Renderer = SceneRenderer::Create(renderer_spec);

	}

	void Scene::OnUpdate(float ts /*= 60.0f*/)
	{
		
	}

	void Scene::FlushDrawList()
	{

	}

}