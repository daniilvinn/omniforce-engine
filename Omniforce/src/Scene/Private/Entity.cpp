#include "../Entity.h"

namespace Omni {

	Entity::Entity(Scene* scene)
		: m_OwnerScene(scene)
	{
		m_Handle = m_OwnerScene->GetRegistry()->create();
	}

	Entity::Entity(entt::entity entity, Scene* scene)
	{
		m_Handle = entity;
		m_OwnerScene = scene;
	}

	Entity::Entity()
	{

	}

}