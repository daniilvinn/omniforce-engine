#pragma once

#include <Foundation/Common.h>
#include <Scene/SceneCommon.h>

#include <Rendering/ISceneRenderer.h>
#include <Rendering/Raster/RasterSceneRenderer.h>
#include <Rendering/PathTracing/PathTracingSceneRenderer.h>
#include <Scene/Sprite.h>
#include <Scene/Component.h>
#include <Core/Serializable.h>
#include <Physics/PhysicsSettings.h>

#include <entt/entt.hpp>
#include <nlohmann/json_fwd.hpp>
#include <robin_hood.h>

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
		// Alternative constructor for template scenes (no renderer)
		Scene(SceneType type, bool create_renderer);

		void Destroy();

		void OnUpdate(float32 step);
		Entity CreateEntity(const UUID& id = UUID());
		Entity CreateEntity(entt::entity entity_id, const UUID& id = UUID());
		Entity CreateChildEntity(Entity parent, const UUID& id = UUID());
		void RemoveEntity(Entity entity);
		void RemoveEntityWithChildren(Entity entity);
		void LaunchRuntime();
		void ShutdownRuntime();
		bool IsInRuntime() const { return m_InRuntime; }
		
		// Scene merging and template functionality
		void MergeScene(const Scene* other_scene, Entity parent);
		bool IsTemplate() const { return !m_Renderer; }
		Ref<Scene> Clone() const;
		Entity InstantiateScene(Ref<Scene> scene, Entity parent);

		SceneType				GetType() const { return m_Type; }
		entt::registry*			GetRegistry() { return &m_Registry; }
		auto&					GetEntities() { return m_Entities; }
		Entity					GetEntity(UUID id) const;
		Entity					GetEntity(std::string_view tag);
		Ref<Image>				GetFinalImage() const { return m_Renderer->GetFinalImage(); }
		Ref<Camera>				GetCamera() const { return m_Camera; };
		WeakPtr<ISceneRenderer>	GetRenderer() const { return m_Renderer; }
		UUID					GetID() const { return m_Id; }
		PhysicsSettings			GetPhysicsSettings() const { return m_PhysicsSettings; }
		void					SetPhysicsSettings(const PhysicsSettings& settings);

		void Serialize(nlohmann::json& node) override;
		void Deserialize(nlohmann::json& node) override;

		/*
		*	Editor only
		*/
		void EditorSetCamera(Ref<Camera> camera) { m_Camera = camera; }

	private:
		// entt component lifecycle hooks
		void OnMeshAdded(entt::registry& registry, entt::entity entity);
		void OnMeshRemoved(entt::registry& registry, entt::entity entity);

		// Helper for recursive entity copying with new UUIDs
		Entity CopyEntityHierarchy(const Scene* source_scene, Entity source_entity, Entity new_parent);

	private:
		UUID m_Id;

		Ref<ISceneRenderer> m_Renderer;
		SceneType m_Type;
		Ref<Camera> m_Camera = nullptr;
		bool m_InRuntime = false;

		entt::registry m_Registry;
		robin_hood::unordered_map<UUID, entt::entity> m_Entities;
		Entity* m_RootNode; // all nodes' parent, origin of the world

		PhysicsSettings m_PhysicsSettings;
	};

}