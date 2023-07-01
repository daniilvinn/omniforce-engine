#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

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
		void SetGravity(fvec3 gravity);

		void LaunchRuntime(Scene* context);
		// Removes (not destroys) all bodies in the physics scene
		void Reset();
		void Update(float32 step);

		// Fetches results into scene
		void FetchResults();

	private:
		PhysicsEngine();
		~PhysicsEngine();

	private:
		static PhysicsEngine* s_Instance;
		JPH::PhysicsSystem* m_CoreSystem;
		PhysicsEngineLimits m_Limits;

		Scene* m_Context = nullptr;
	};

}