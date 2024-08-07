#pragma once

#include "SceneCommon.h"
#include "SceneRenderer.h"
#include "Sprite.h"
#include "Core/UUID.h"
#include "Core/Serializable.h"
#include "Physics/PhysicsSettings.h"
#include "Component.h"

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
		Scene(Scene* other);

		void Destroy();

		void OnUpdate(float32 step);
		Entity CreateEntity(const UUID& id = UUID());
		Entity CreateEntity(entt::entity entity_id, const UUID& id = UUID());
		Entity CreateChildEntity(Entity parent, const UUID& id = UUID());
		void RemoveEntity(Entity entity);
		void RemoveEntityWithChildren(Entity entity);
		void LaunchRuntime();
		void ShutdownRuntime();
		fvec3 TraverseSceneHierarchy(Entity node, TRSComponent origin);
		

		SceneType				GetType() const { return m_Type; }
		entt::registry*			GetRegistry() { return &m_Registry; }
		auto&					GetEntities() { return m_Entities; }
		Entity					GetEntity(UUID id);
		Entity					GetEntity(std::string_view tag);
		Shared<Image>			GetFinalImage() const { return m_Renderer->GetFinalImage(); }
		Shared<Camera>			GetCamera() const { return m_Camera; };
		Shared<SceneRenderer>	GetRenderer() const { return m_Renderer; }
		UUID					GetID() const { return m_Id; }
		PhysicsSettings			GetPhysicsSettings() const { return m_PhysicsSettings; }
		void					SetPhysicsSettings(const PhysicsSettings& settings);

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
		bool m_InRuntime = false;

		entt::registry m_Registry;
		robin_hood::unordered_map<UUID, entt::entity> m_Entities;
		Entity* m_RootNode; // all nodes' parent, origin of the world

		PhysicsSettings m_PhysicsSettings;
	};

}