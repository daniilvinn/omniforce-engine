#pragma once

#include "SceneCommon.h"
#include "SceneRenderer.h"
#include "Sprite.h"

#include <entt/entt.hpp>

namespace Omni {

	enum class OMNIFORCE_API SceneType : uint8 {
		SCENE_TYPE_2D,
		SCENE_TYPE_3D,
	};

	class Entity;

	class OMNIFORCE_API Scene {
	public:
		Scene() = delete;
		Scene(SceneType type);

		void Destroy();

		void OnUpdate(float ts = 60.0f);
		Entity CreateEntity(const std::string& name = "Entity");

		entt::registry* GetRegistry() { return &m_Registry; }
		std::vector<Entity>& GetEntities() { return m_Entities; }

	private:
		SceneRenderer* m_Renderer;
		SceneType m_Type;
		Shared<Camera> m_Camera;

		entt::registry m_Registry;
		std::vector<Entity> m_Entities;

	};

}