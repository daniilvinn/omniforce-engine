#pragma once

#include <Foundation/Common.h>
#include <Scene/SceneCommon.h>

#include <Scene/Scene.h>
#include <Core/Serializable.h>

#include <entt/entt.hpp>

namespace Omni {

	class OMNIFORCE_API Entity : public Serializable {
	public:
		Entity();
		Entity(Scene* scene);
		Entity(entt::entity entity, Scene* scene);

		operator entt::entity() const { return m_Handle; }

		template<typename Component, typename... Args>
		Component& AddComponent(Args&&... args) {
			return m_OwnerScene->GetRegistry()->emplace<Component>(m_Handle, std::forward<Args>(args)...);
		};

		template<typename Component>
		Component& GetComponent() {
			OMNIFORCE_ASSERT_TAGGED(m_Handle != entt::null && m_OwnerScene, "Entity is not constructed");
			OMNIFORCE_ASSERT_TAGGED(HasComponent<Component>(), "Component not found");
			return m_OwnerScene->GetRegistry()->get<Component>(m_Handle);
		}

		template<typename Component>
		void RemoveComponent() {
			m_OwnerScene->GetRegistry()->remove<Component>(m_Handle);
		}

		template <typename Component>
		bool HasComponent() {
			return (bool)m_OwnerScene->GetRegistry()->try_get<Component>(m_Handle);
		}

		inline operator bool() {
			return m_Handle != entt::null;
		}

		inline bool operator==(const Entity& other) const {
			return (m_Handle == other.m_Handle) && (m_OwnerScene == other.m_OwnerScene);
		}

		bool Valid() const { return m_Handle != entt::null; }

		void Invalidate() { // make this entity handle no longer valid
			m_Handle = entt::null;
			m_OwnerScene = nullptr;
		}

		UUID GetID();
		TRSComponent GetWorldTransform();
		Entity GetParent() {
			UUID parent_id = GetComponent<HierarchyNodeComponent>().parent;
			return parent_id.Valid() ? m_OwnerScene->GetEntity(parent_id) : Entity();
		}

		std::vector<UUID>& Children() {
			return GetComponent<HierarchyNodeComponent>().children;
		}

		bool HasInHierarchy(Entity entity) {
			std::vector<UUID>& children = Children();

			if (children.empty())
				return false;

			for (UUID child : children)
			{
				if (child == entity.GetID())
					return true;
			}

			for (UUID child : children)
			{
				if (m_OwnerScene->GetEntity(child).HasInHierarchy(entity))
					return true;
			}

			return false;
		}

		void Serialize(nlohmann::json& node) override;
		void Deserialize(nlohmann::json& node) override;


	private:
		entt::entity m_Handle;
		Scene* m_OwnerScene;
	};

}