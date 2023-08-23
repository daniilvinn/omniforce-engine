#include "JoltUtils.h"

#include <Foundation/Types.h>
#include <Log/Logger.h>
#include <Scripting/ScriptEngine.h>
#include <Scripting/PendingCallbackInfo.h>
#include <Scene/Scene.h>
#include <Scene/Component.h>
#include "../PhysicsMaterial.h"

#include <cstdarg>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/Body.h>

namespace Omni {

	void PhysicsEngineTraceImpl(const char* fmt, ...)
	{
		// Format the message
		va_list list;
		va_start(list, fmt);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), fmt, list);
		va_end(list);

		OMNIFORCE_CORE_TRACE("[Physics engine internal]: {}", buffer);
		
	}

	bool PhysicsEngineAssertFailedImpl(const char* expression, const char* message, const char* file, uint32 line) {
		OMNIFORCE_CORE_CRITICAL("[Physics engine internal]: Assertion failed with expression {0} ({2}, line {3}): {1}",
			expression, message, file, line
		);
		
		return true;
	}

	void GetFrictionAndRestitution(const JPH::Body& body, const JPH::SubShapeID& subshape_id, float32* out_friction, float32* out_restitution)
	{
		const JPH::PhysicsMaterial* physics_material = body.GetShape()->GetMaterial(subshape_id);

		if (physics_material == JPH::PhysicsMaterial::sDefault) {
			*out_friction = body.GetFriction();
			*out_restitution = body.GetRestitution();
		}
		else {
			const PhysicsMaterial* omni_physics_material = (PhysicsMaterial*)physics_material;
			*out_friction = omni_physics_material->m_Friction;
			*out_restitution = omni_physics_material->m_Restitution;
		}
	}

	bool ObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer object1, JPH::ObjectLayer object2) const
	{
		switch (object1)
		{
		case BodyLayers::NON_MOVING:			return object2 == BodyLayers::MOVING;
		case BodyLayers::MOVING:				return true;
		default:								std::unreachable();
		}
	}

	BroadPhaseLayerInterface::BroadPhaseLayerInterface()
	{
		m_ObjectToBroadPhase[BodyLayers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		m_ObjectToBroadPhase[BodyLayers::MOVING] = BroadPhaseLayers::MOVING;
	}

	uint32 BroadPhaseLayerInterface::GetNumBroadPhaseLayers() const
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	JPH::BroadPhaseLayer BroadPhaseLayerInterface::GetBroadPhaseLayer(JPH::ObjectLayer layer) const
	{
		return m_ObjectToBroadPhase[layer];
	}

	const char* BroadPhaseLayerInterface::GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const
	{
		switch ((JPH::BroadPhaseLayer::Type)layer)
		{
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:														std::unreachable();
		}
	}

	bool ObjectBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
	{
		switch (inLayer1)
		{
		case BodyLayers::NON_MOVING:		return inLayer2 == BroadPhaseLayers::MOVING;
		case BodyLayers::MOVING:			return true;
		default:							OMNIFORCE_ASSERT(false); std::unreachable();
		}
	}

	BodyContantListener::BodyContantListener()
	{
		m_ScriptEngine = ScriptEngine::Get();
	}

	JPH::ValidateResult BodyContantListener::OnContactValidate(
		const JPH::Body& inBody1, 
		const JPH::Body& inBody2, 
		JPH::RVec3Arg inBaseOffset, 
		const JPH::CollideShapeResult& inCollisionResult
	)
	{
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void BodyContantListener::OnContactAdded(
		const JPH::Body& body1, 
		const JPH::Body& body2, 
		const JPH::ContactManifold& manifold, 
		JPH::ContactSettings& settings
	)
	{
		float32 friction1, friction2, restitution1, restitution2;
		GetFrictionAndRestitution(body1, manifold.mSubShapeID1, &friction1, &restitution1);
		GetFrictionAndRestitution(body2, manifold.mSubShapeID2, &friction2, &restitution2);

		settings.mCombinedFriction = std::sqrt(friction1 * friction2);
		settings.mCombinedRestitution = std::max(restitution1, restitution2);

		Scene* scene_context = m_ScriptEngine->Get()->GetContext();
		auto& entities = scene_context->GetEntities();
		Entity entity1(entities[body1.GetUserData()], scene_context);
		Entity entity2(entities[body2.GetUserData()], scene_context);

		if (entity1.HasComponent<ScriptComponent>()) {
			TransientAllocator<false>& args_allocator = m_ScriptEngine->GetCallbackArgsAllocator();
			UUID* uuid = args_allocator.Allocate<UUID>(body2.GetUserData());

			PendingCallbackInfo callback_info = {};
			callback_info.entity = entity1;
			callback_info.args = { uuid };
			callback_info.method = "OnContactAdded";

			m_ScriptEngine->AddPendingCallback(callback_info);
		}

		if (entity2.HasComponent<ScriptComponent>()) {
			TransientAllocator<false>& args_allocator = m_ScriptEngine->GetCallbackArgsAllocator();
			UUID* uuid = args_allocator.Allocate<UUID>(body1.GetUserData());

			PendingCallbackInfo callback_info = {};
			callback_info.entity = entity2;
			callback_info.args = { uuid };
			callback_info.method = "OnContactAdded";

			m_ScriptEngine->AddPendingCallback(callback_info);
		}
	}

	void BodyContantListener::OnContactPersisted(
		const JPH::Body& body1, 
		const JPH::Body& body2, 
		const JPH::ContactManifold& manifold, 
		JPH::ContactSettings& settings
	)
	{
		float32 friction1, friction2, restitution1, restitution2;
		GetFrictionAndRestitution(body1, manifold.mSubShapeID1, &friction1, &restitution1);
		GetFrictionAndRestitution(body2, manifold.mSubShapeID2, &friction2, &restitution2);

		settings.mCombinedFriction = std::sqrt(friction1 * friction2);
		settings.mCombinedRestitution = std::max(restitution1, restitution2);

		Scene* scene_context = m_ScriptEngine->Get()->GetContext();
		auto& entities = scene_context->GetEntities();
		Entity entity1(entities[body1.GetUserData()], scene_context);
		Entity entity2(entities[body2.GetUserData()], scene_context);

		if (entity1.HasComponent<ScriptComponent>()) {
			TransientAllocator<false>& args_allocator = m_ScriptEngine->GetCallbackArgsAllocator();
			UUID* uuid = args_allocator.Allocate<UUID>(body2.GetUserData());

			PendingCallbackInfo callback_info = {};
			callback_info.entity = entity1;
			callback_info.args = { uuid };
			callback_info.method = "OnContactPersisted";

			m_ScriptEngine->AddPendingCallback(callback_info);
		}

		if (entity2.HasComponent<ScriptComponent>()) {
			TransientAllocator<false>& args_allocator = m_ScriptEngine->GetCallbackArgsAllocator();
			UUID* uuid = args_allocator.Allocate<UUID>(body1.GetUserData());

			PendingCallbackInfo callback_info = {};
			callback_info.entity = entity2;
			callback_info.args = { uuid };
			callback_info.method = "OnContactPersisted";

			m_ScriptEngine->AddPendingCallback(callback_info);
		}
	}

	void BodyContantListener::OnContactRemoved(const JPH::SubShapeIDPair& subshape_pair)
	{
		
	}

}