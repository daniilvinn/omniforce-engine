#pragma once

#include <Foundation/Common.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>

namespace Omni {

	class PhysicsMaterial : public JPH::PhysicsMaterial {
	public:
		PhysicsMaterial(float32 friction, float32 restitution) : m_Friction(friction), m_Restitution(restitution) {};

		float32 m_Friction;
		float32 m_Restitution;
	};

}