#pragma once

#include "SceneCommon.h"
#include "SceneRenderer.h"
#include "Sprite.h"
#include "Core/UUID.h"
#include "Core/Serializable.h"

#include <entt/entt.hpp>
#include <nlohmann/json_fwd.hpp>

namespace Omni {

	enum class OMNIFORCE_API SceneType : uint8 {
		SCENE_TYPE_2D,
		SCENE_TYPE_3D,
	};

	class Entity;

	class OMNIFORCE_API Scene : public Serializable {
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

		void Serialize(nlohmann::json& node) override;
		void Deserialize(nlohmann::json& node) override;

		/*
		*	Editor only
		*/
		void EditorSetCamera(Shared<Camera> camera) { m_Camera = camera; }

	private:
		UUID m_Id;

		SceneRenderer* m_Renderer;
		SceneType m_Type;
		Shared<Camera> m_Camera;

		entt::registry m_Registry;
		std::vector<Entity> m_Entities;
	};

}