#pragma once

#include <Foundation/Types.h>
#include <Log/Logger.h>
#include <Foundation/Macros.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace Omni {

	class OMNIFORCE_API ScriptEngine;
	class OMNIFORCE_API Scene;

	void PhysicsEngineTraceImpl(const char* fmt, ...);
	bool PhysicsEngineAssertFailedImpl(const char* expression, const char* message, const char* file, uint32 line);

	namespace BodyLayers
	{
		static constexpr JPH::ObjectLayer MOVING = 0;
		static constexpr JPH::ObjectLayer NON_MOVING = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
	};

	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr uint32 NUM_LAYERS(2);
	};


	/// Class that determines if two object layers can collide
	class ObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer object1, JPH::ObjectLayer object2) const override;
	};

	class BroadPhaseLayerInterface : public JPH::BroadPhaseLayerInterface {
	public:
		BroadPhaseLayerInterface();

		virtual uint32 GetNumBroadPhaseLayers() const override;
		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override;
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override;

	private:
		JPH::BroadPhaseLayer m_ObjectToBroadPhase[BodyLayers::NUM_LAYERS];
	};

	class ObjectBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
	};

	class BodyContantListener : public JPH::ContactListener
	{
	public:
		BodyContantListener();

		virtual JPH::ValidateResult	OnContactValidate(
			const JPH::Body& inBody1, 
			const JPH::Body& inBody2, 
			JPH::RVec3Arg inBaseOffset, 
			const JPH::CollideShapeResult& inCollisionResult) override;

		virtual void OnContactAdded(
			const JPH::Body& body1, 
			const JPH::Body& body2, 
			const JPH::ContactManifold& manifold, 
			JPH::ContactSettings& settings) override;

		virtual void OnContactPersisted(
			const JPH::Body& body1, 
			const JPH::Body& body2, 
			const JPH::ContactManifold& manifold, 
			JPH::ContactSettings& settings) override;

		virtual void OnContactRemoved(const JPH::SubShapeIDPair& subshape_pair) override;

	private:
		ScriptEngine* m_ScriptEngine;
	};

	class BodyActivationListener : public JPH::BodyActivationListener
	{
	public:
		virtual void OnBodyActivated(const JPH::BodyID& inBodyID, uint64 inBodyUserData) override {}
		virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, uint64 inBodyUserData) override {}
	};

	void GetFrictionAndRestitution(
		const JPH::Body& body,
		const JPH::SubShapeID& subshape_id,
		float32* out_friction,
		float32* out_restitution
	);

}