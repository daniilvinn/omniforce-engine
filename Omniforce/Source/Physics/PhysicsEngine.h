#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Core/Timer.h>
#include <Scene/Entity.h>
#include "PhysicsSettings.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace Omni {

	class Scene;

	struct OMNIFORCE_API PhysicsEngineLimits {
		const uint32 MAX_BODIES = UINT16_MAX;
		const uint32 MAX_BROAD_PHASE_BODY_PAIRS = UINT16_MAX;
		const uint32 MAX_CONTACT_CONSTRAINTS = UINT16_MAX / 2;
		const uint32 MAX_BODY_MUTEXES = 0;
	};

	class OMNIFORCE_API PhysicsEngine {
	public:
		static void Init();
		static void Shutdown();
		static PhysicsEngine* Get() { return s_Instance; }

		const PhysicsEngineLimits& GetLimits() const { return m_Limits; }
		bool HasContext() const { return m_Context != nullptr; }
		void SetGravity(fvec3 gravity);
		PhysicsSettings GetSettings() const { return m_Settings; }
		void SetSettings(const PhysicsSettings& settings);

		void LaunchRuntime(Scene* context);
		void Reset();
		void Update();
		void FetchResults();

		fvec3 GetLinearVelocity(Entity entity);
		void SetLinearVelocity(Entity entity, fvec3 velocity);
		void AddForce(Entity entity, fvec3 force);
		void AddTorque(Entity entity, fvec3 torque);
		void AddLinearImpulse(Entity entity, fvec3 impulse);

		JPH::BodyInterface& GetBodyInterface() { return m_CoreSystem->GetBodyInterface(); }
		JPH::BodyInterface& GetBodyInterfaceNoLock() { return m_CoreSystem->GetBodyInterfaceNoLock(); }

	private:
		PhysicsEngine();
		~PhysicsEngine();

	private:
		static PhysicsEngine* s_Instance;
		JPH::PhysicsSystem* m_CoreSystem;
		PhysicsEngineLimits m_Limits;
		PhysicsSettings m_Settings;

		Scene* m_Context = nullptr;
		float32 m_TimeSinceLastUpdate = 1 / 121.0f;
		Timer m_Timer;
	};

}