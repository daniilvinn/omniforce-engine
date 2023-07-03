#pragma once

#include "SceneCommon.h"
#include "SceneRenderer.h"
#include "Sprite.h"
#include "Core/UUID.h"
#include "Core/Serializable.h"

#include <entt/entt.hpp>
#include <nlohmann/json_fwd.hpp>
#include <robin_hood.h>

#include <Core/UUID.h>

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
		Scene(Scene* other); // copy

		void Destroy();

		void OnUpdate(float32 step);
		Entity CreateEntity(const UUID& id = UUID());
		void LaunchRuntime();
		void ShutdownRuntime();

		SceneType GetType() const { return m_Type; }
		entt::registry* GetRegistry() { return &m_Registry; }
		robin_hood::unordered_map<UUID, entt::entity>& GetEntities() { return m_Entities; }
		Shared<Image> GetFinalImage() const { return m_Renderer->GetFinalImage(); }
		Shared<Camera> GetCamera() const { return m_Camera; };
		Shared<SceneRenderer> GetRenderer() const { return m_Renderer; }
		UUID GetID() const { return m_Id; }

		void Serialize(nlohmann::json& node) override;
		void Deserialize(nlohmann::json& node) override;

		/*
		*	Editor only
		*/
		void EditorSetCamera(Shared<Camera> camera) { m_Camera = camera; }

	private:
		UUID m_Id;

		Shared<SceneRenderer> m_Renderer;
		SceneType m_Type;
		Shared<Camera> m_Camera = nullptr;

		entt::registry m_Registry;
		robin_hood::unordered_map<UUID, entt::entity> m_Entities;
	};

}