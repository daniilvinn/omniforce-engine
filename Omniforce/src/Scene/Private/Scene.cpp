#include "../Scene.h"
#include "../Camera.h"
#include "SceneRenderer.h"

namespace Omni {

	Scene::Scene(SceneType type)
		: m_Type(type)
	{
		switch (m_Type)
		{
		case SceneType::SCENE_TYPE_2D:		m_Camera = new Camera2D;
		case SceneType::SCENE_TYPE_3D:		m_Camera = new Camera3D;
		default:							std::unreachable();
		}
	}

	void Scene::OnUpdate(float ts /*= 60.0f*/)
	{

	}

	void Scene::FlushDrawList()
	{

	}

}