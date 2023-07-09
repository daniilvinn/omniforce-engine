#pragma once

#include "SceneCommon.h"
#include "Scene.h"

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
			OMNIFORCE_ASSERT_TAGGED(HasComponent<Component>(), "No component found");
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

		void Serialize(nlohmann::json& node) override;
		void Deserialize(nlohmann::json& node) override;

	private:
		entt::entity m_Handle;
		Scene* m_OwnerScene;
	};

}