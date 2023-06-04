#pragma once

#include "SceneCommon.h"
#include "SceneRenderer.h"
#include "Sprite.h"
#include "Core/UUID.h"

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
		Entity CreateEntity(const UUID& id = UUID());

		entt::registry* GetRegistry() { return &m_Registry; }
		std::vector<Entity>& GetEntities() { return m_Entities; }
		Shared<Image> GetFinalImage() const { return m_Renderer->GetFinalImage(); }
		Shared<Camera> GetCamera() const { return m_Camera; };
		SceneRenderer* GetRenderer() const { return m_Renderer; }
		UUID GetID() const { return m_Id; }

	private:
		UUID m_Id;

		SceneRenderer* m_Renderer;
		SceneType m_Type;
		Shared<Camera> m_Camera;

		entt::registry m_Registry;
		std::vector<Entity> m_Entities;
	};

}