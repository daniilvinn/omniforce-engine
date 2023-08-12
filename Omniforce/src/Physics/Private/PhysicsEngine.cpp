#include "../PhysicsEngine.h"
#include "../PhysicsMaterial.h"
#include <Scene/Scene.h>
#include <Scene/Component.h>
#include <Scene/Entity.h>
#include <Core/Input/Input.h>

#include "JoltUtils.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsScene.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Core/TempAllocator.h>

#include <glm/gtc/quaternion.hpp>

namespace Omni {

	PhysicsEngine* PhysicsEngine::s_Instance = nullptr;

	constexpr JPH::EMotionType convert(RigidBody2DComponent::Type type) {
		switch (type)
		{
		case RigidBody2DComponent::Type::STATIC:			return JPH::EMotionType::Static;
		case RigidBody2DComponent::Type::DYNAMIC:			return JPH::EMotionType::Dynamic;
		case RigidBody2DComponent::Type::KINEMATIC:			return JPH::EMotionType::Kinematic;
		default:											std::unreachable();
		}
	}

	struct PhysicsEngineInternals {
		BroadPhaseLayerInterface broad_phase_layer_interface;
		ObjectBroadPhaseLayerFilter object_vs_broad_phase_layer_filter;
		ObjectLayerPairFilter object_layer_pair_filter;
		BodyActivationListener body_activation_listener;
		BodyContantListener body_contact_listener;
		JPH::TempAllocatorImpl* temp_allocator;
		JPH::JobSystemThreadPool* job_system;
	} s_InternalData;

	PhysicsEngine::PhysicsEngine()
	{
		JPH::RegisterDefaultAllocator();

		JPH::Trace = PhysicsEngineTraceImpl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = PhysicsEngineAssertFailedImpl);

		JPH::Factory::sInstance = new JPH::Factory;
		JPH::RegisterTypes();

		s_InternalData.body_contact_listener = BodyContantListener();
		m_CoreSystem = new JPH::PhysicsSystem;

		m_CoreSystem->Init(
			m_Limits.MAX_BODIES,
			m_Limits.MAX_BODY_MUTEXES,
			m_Limits.MAX_BROAD_PHASE_BODY_PAIRS,
			m_Limits.MAX_CONTACT_CONSTRAINTS,
			s_InternalData.broad_phase_layer_interface,
			s_InternalData.object_vs_broad_phase_layer_filter,
			s_InternalData.object_layer_pair_filter
		);

		m_CoreSystem->SetBodyActivationListener(&s_InternalData.body_activation_listener);
		m_CoreSystem->SetContactListener(&s_InternalData.body_contact_listener);

		// allocate 50 mb for physics engine temporal data
		s_InternalData.temp_allocator = new JPH::TempAllocatorImpl(50 * 1024 * 1024);

		// for now I let Jolt to use all available threads except main thread. 
		// Later I will need to switch to my custom job system to handle Jolt jobs.
		s_InternalData.job_system = new JPH::JobSystemThreadPool(
			JPH::cMaxPhysicsJobs,
			JPH::cMaxPhysicsBarriers,
			std::thread::hardware_concurrency() - 1
		);

		OMNIFORCE_CORE_INFO("Initialized physics engine: ");
		OMNIFORCE_CORE_INFO("\tMax rigid bodies: {}", m_Limits.MAX_BODIES);
		OMNIFORCE_CORE_INFO("\tMax body mutexes: {} {}", m_Limits.MAX_BODY_MUTEXES, m_Limits.MAX_BODY_MUTEXES ? "" : "(default)");
		OMNIFORCE_CORE_INFO("\tMax broad phase body pairs: {}", m_Limits.MAX_BROAD_PHASE_BODY_PAIRS);
		OMNIFORCE_CORE_INFO("\tMax contact constraints: {}", m_Limits.MAX_CONTACT_CONSTRAINTS);
	}

	PhysicsEngine::~PhysicsEngine()
	{
		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
		delete s_InternalData.temp_allocator;
		delete s_InternalData.job_system;
	}

	void PhysicsEngine::Init()
	{
		s_Instance = new PhysicsEngine;
	}

	void PhysicsEngine::Shutdown()
	{
		delete s_Instance;
	}

	void PhysicsEngine::SetGravity(fvec3 gravity)
	{
		m_Settings.gravity = gravity;
		m_CoreSystem->SetGravity({ gravity.x, gravity.y, gravity.z });
	}

	void PhysicsEngine::SetSettings(const PhysicsSettings& settings)
	{
		m_Settings = settings;
		SetGravity(m_Settings.gravity);
	}

	void PhysicsEngine::LaunchRuntime(Scene* context)
	{
		m_Context = context;

		// Create and add Jolt bodies
		auto registry = context->GetRegistry();
		auto rb2d_view = registry->view<RigidBody2DComponent>();
		JPH::BodyInterface& body_interface = m_CoreSystem->GetBodyInterface();

		for (auto& e : rb2d_view) {
			Entity entity(e, context);
			auto& rb2d_component = entity.GetComponent<RigidBody2DComponent>();
			
			JPH::BodyCreationSettings body_creation_settings;
			body_creation_settings.mGravityFactor = (float32)!rb2d_component.disable_gravity;
			body_creation_settings.mAllowedDOFs = JPH::EAllowedDOFs::Plane2D;
			body_creation_settings.mMotionType = convert(rb2d_component.type);
			body_creation_settings.mObjectLayer = rb2d_component.type == RigidBody2DComponent::Type::STATIC ? BodyLayers::NON_MOVING : BodyLayers::MOVING;
			body_creation_settings.mLinearDamping = rb2d_component.linear_drag;
			body_creation_settings.mAngularDamping = rb2d_component.angular_drag;
			body_creation_settings.mIsSensor = rb2d_component.sensor_mode;
			body_creation_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
			body_creation_settings.mMassPropertiesOverride.mMass = rb2d_component.mass;

			// TODO: optimize caching by adding arena allocator for body IDs
			JPH::BodyID* body_id = new JPH::BodyID;

			if (entity.HasComponent<BoxColliderComponent>()) {
				BoxColliderComponent& box_collider_component = entity.GetComponent<BoxColliderComponent>();
				JPH::BoxShapeSettings box_shape_settings(
					{
						box_collider_component.size.x,
						box_collider_component.size.y,
						box_collider_component.size.z,
					},
					box_collider_component.convex_radius,
					(const JPH::PhysicsMaterial*)new PhysicsMaterial(
						box_collider_component.friction, 
						box_collider_component.restitution
					)
				);
				// ngl I hope really much that I will not get an error while creating shapes
				auto& trs = entity.GetComponent<TRSComponent>();
				body_creation_settings.mPosition = { trs.translation.x, trs.translation.y, trs.translation.z };

				glm::quat q({ glm::radians(trs.rotation.x), glm::radians(trs.rotation.y), glm::radians(trs.rotation.z) });
				body_creation_settings.mRotation = JPH::Quat(q.x, q.y, q.z, q.w);

				body_creation_settings.mUserData = entity.GetComponent<UUIDComponent>().id.Get();
				JPH::ShapeSettings::ShapeResult shape_result = box_shape_settings.Create();
				OMNIFORCE_ASSERT_TAGGED(!shape_result.HasError(), shape_result.GetError());
				body_creation_settings.SetShape(shape_result.Get());
				*body_id = body_interface.CreateAndAddBody(body_creation_settings, JPH::EActivation::Activate);
			}
			else if (entity.HasComponent<SphereColliderComponent>()) {
				SphereColliderComponent& sphere_collider_component = entity.GetComponent<SphereColliderComponent>();
				JPH::SphereShapeSettings sphere_shape_settings(
					sphere_collider_component.radius,
					(const JPH::PhysicsMaterial*)new PhysicsMaterial(
						sphere_collider_component.friction,
						sphere_collider_component.restitution
					)
				);

				auto& trs = entity.GetComponent<TRSComponent>();
				body_creation_settings.mPosition = { trs.translation.x, trs.translation.y, trs.translation.z };

				glm::quat q({ glm::radians(trs.rotation.x), glm::radians(trs.rotation.y), glm::radians(trs.rotation.z) });
				body_creation_settings.mRotation = JPH::Quat(q.x, q.y, q.z, q.w);
				body_creation_settings.mUserData = entity.GetComponent<UUIDComponent>().id.Get();
				body_creation_settings.SetShape(sphere_shape_settings.Create().Get());
				*body_id = body_interface.CreateAndAddBody(body_creation_settings, JPH::EActivation::Activate);
			}
			rb2d_component.internal_body = body_id;
		}

		m_CoreSystem->OptimizeBroadPhase();
		m_Timer.Reset();
	}

	void PhysicsEngine::Reset()
	{
		JPH::BodyInterface& body_interface = m_CoreSystem->GetBodyInterface();
		JPH::BodyIDVector bodies_ids; 
		m_CoreSystem->GetBodies(bodies_ids);
		
		if (bodies_ids.size()) {
			body_interface.RemoveBodies(bodies_ids.data(), bodies_ids.size());
			body_interface.DestroyBodies(bodies_ids.data(), bodies_ids.size());
		}

		m_Context = nullptr;
	}

	void PhysicsEngine::Update()
	{
		// fetch new objects' position which was changed from scripts
		auto group = m_Context->GetRegistry()->group<RigidBody2DComponent, ScriptComponent>(entt::get<TRSComponent>);
		JPH::BodyInterface& body_interface = m_CoreSystem->GetBodyInterface();

		for (auto& e : group) {
			Entity entity(e, m_Context);

			TRSComponent& trs = entity.GetComponent<TRSComponent>();
			RigidBody2DComponent& rb2d_component = entity.GetComponent<RigidBody2DComponent>();
			
			body_interface.SetPosition(
				*(JPH::BodyID*)rb2d_component.internal_body, 
				{ trs.translation.x, trs.translation.y, trs.translation.z }, 
				JPH::EActivation::Activate
			);
 		}
		
		// update
		int32 optimal_collision_steps = 1;
		float32 time_since_last_update = m_Timer.ElapsedMilliseconds();
		if (time_since_last_update > (1000.0f / 120.0f)) {
			uint32 required_physics_update_iterations = (int32)(time_since_last_update / (1000.0f / 120.0f));

			for (uint32 i = 0; i < required_physics_update_iterations; i++)
				m_CoreSystem->Update(1 / 120.0f, optimal_collision_steps, s_InternalData.temp_allocator, s_InternalData.job_system);
			m_Timer.Reset();
		}
	}

	void PhysicsEngine::FetchResults()
	{
		JPH::BodyIDVector bodies;
		m_CoreSystem->GetBodies(bodies);

		JPH::BodyInterface& body_interface = m_CoreSystem->GetBodyInterface();
		auto& entities = m_Context->GetEntities();

		for (auto& id : bodies) {
			UUID entity_id = body_interface.GetUserData(id);
			Entity entity(entities.at(entity_id), m_Context);

			TRSComponent& trs_component = entity.GetComponent<TRSComponent>();
			JPH::RVec3 position = body_interface.GetPosition(id);
			JPH::Quat rotation = body_interface.GetRotation(id);

			trs_component.translation = { position.GetX(),position.GetY(),position.GetZ() };

			glm::vec3 euler_angles = glm::eulerAngles(glm::quat( rotation.GetX(), rotation.GetY(),rotation.GetZ(),rotation.GetW()));
			trs_component.rotation = { 0.0, 0.0, glm::degrees(euler_angles.x) };
		}
	}

	void PhysicsEngine::AddLinearImpulse(Entity entity, fvec3 impulse)
	{
		JPH::BodyInterface& body_interface = m_CoreSystem->GetBodyInterface();
		body_interface.AddImpulse(
			*(JPH::BodyID*)entity.GetComponent<RigidBody2DComponent>().internal_body, 
			{ impulse.x, impulse.y, impulse.z }
		);
	}

	fvec3 PhysicsEngine::GetLinearVelocity(Entity entity)
	{
		RigidBody2DComponent rb2d_component = entity.GetComponent<RigidBody2DComponent>();
		JPH::BodyID body_id = *(JPH::BodyID*)rb2d_component.internal_body;

		JPH::Vec3 velocity = m_CoreSystem->GetBodyInterface().GetLinearVelocity(body_id);

		return { velocity.GetX(), velocity.GetY(), velocity.GetZ() };
	}


	void PhysicsEngine::SetLinearVelocity(Entity entity, fvec3 velocity)
	{
		RigidBody2DComponent rb2d_component = entity.GetComponent<RigidBody2DComponent>();
		JPH::BodyID body_id = *(JPH::BodyID*)rb2d_component.internal_body;

		JPH::Vec3 value(velocity.x, velocity.y, velocity.z);

		m_CoreSystem->GetBodyInterface().SetLinearVelocity(body_id, value);
	}

	void PhysicsEngine::AddForce(Entity entity, fvec3 force)
	{
		RigidBody2DComponent rb2d_component = entity.GetComponent<RigidBody2DComponent>();
		JPH::BodyID body_id = *(JPH::BodyID*)rb2d_component.internal_body;

		JPH::Vec3 value(force.x, force.y, force.z);
		m_CoreSystem->GetBodyInterface().AddForce(body_id, value);
	}

	void PhysicsEngine::AddTorque(Entity entity, fvec3 torque)
	{
		RigidBody2DComponent rb2d_component = entity.GetComponent<RigidBody2DComponent>();
		JPH::BodyID body_id = *(JPH::BodyID*)rb2d_component.internal_body;

		JPH::Vec3 value(torque.x, torque.y, torque.z);
		m_CoreSystem->GetBodyInterface().AddTorque(body_id, value);
	}

}