#include "ScriptAPI.h"

#include <Core/Application.h>
#include <Core/Input/Input.h>
#include <Core/UUID.h>
#include <Log/Logger.h>
#include <Scene/Component.h>
#include <Scripting/ScriptEngine.h>
#include <Physics/PhysicsEngine.h>
#include <Audio/AudioEngine.h>

#include <mono/jit/jit.h>

#define OMNI_REGISTER_SCRIPT_API_FUNCTION(name) mono_add_internal_call("Omni.EngineAPI::" #name, name)

namespace Omni {

	bool Input_KeyPressed(KeyCode code) {
		return Input::KeyPressed(code);
	}

	float Input_GetDeltaTime() {
		return Application::Get()->GetDeltaTime();
	}

	glm::vec3 TransformComponent_GetTranslation(uint64 entity_id) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		return entity.GetComponent<TRSComponent>().translation;
	}

	void TransformComponent_SetTranslation(uint64 entity_id, glm::vec3 translation) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		entity.GetComponent<TRSComponent>().translation = translation;
	} 

	glm::quat TransformComponent_GetRotation(uint64 entity_id) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		return entity.GetComponent<TRSComponent>().rotation;
	}

	void TransformComponent_SetRotation(uint64 entity_id, glm::quat rotation) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		entity.GetComponent<TRSComponent>().rotation = rotation;
	}

	glm::vec3 TransformComponent_GetScale(uint64 entity_id) 
	{
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		return entity.GetComponent<TRSComponent>().scale;
	}

	void TransformComponent_SetScale(uint64 entity_id, glm::vec3 scale) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		entity.GetComponent<TRSComponent>().scale = scale;
	}

	void Logger_Log(Logger::Level severity, MonoString* message) {
		char* msg = mono_string_to_utf8(message);
		switch (severity)
		{
		case Logger::Level::LEVEL_TRACE:			OMNIFORCE_CUSTOM_LOGGER_TRACE("Game", msg);			break;
		case Logger::Level::LEVEL_INFO:				OMNIFORCE_CUSTOM_LOGGER_INFO("Game", msg);			break;
		case Logger::Level::LEVEL_WARN:				OMNIFORCE_CUSTOM_LOGGER_WARN("Game", msg);			break;
		case Logger::Level::LEVEL_ERROR:			OMNIFORCE_CUSTOM_LOGGER_ERROR("Game", msg);			break;
		case Logger::Level::LEVEL_CRITICAL:			OMNIFORCE_CUSTOM_LOGGER_CRITICAL("Game", msg);		break;
		case Logger::Level::LEVEL_NONE:																	break;
		default:									std::unreachable();									break;
		}
		mono_free(msg);
	}

	MonoString* TagComponent_GetTag(uint64 entity_id) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		return mono_string_new(script_engine->GetDomain(), entity.GetComponent<TagComponent>().tag.c_str());
	}

	void RigidBody2D_AddLinearImpulse(uint64 entity_id, fvec3* impulse) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		PhysicsEngine::Get()->AddLinearImpulse(entity, *impulse);
	}

	void RigidBody2D_GetLinearVelocity(uint64 entity_id, fvec3* value) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);

		*value = PhysicsEngine::Get()->GetLinearVelocity(entity);
	}

	void RigidBody2D_SetLinearVelocity(uint64 entity_id, fvec3* value) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);

		PhysicsEngine::Get()->SetLinearVelocity(entity, *value);
	}

	void RigidBody2D_AddForce(uint64 entity_id, fvec3* value) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);

		PhysicsEngine::Get()->AddForce(entity, *value);
	}

	void RigidBody2D_AddTorque(uint64 entity_id, fvec3* value) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);

		PhysicsEngine::Get()->AddTorque(entity, *value);
	}

	void Physics_GetGravity(fvec3* value) {
		*value = ScriptEngine::Get()->GetContext()->GetPhysicsSettings().gravity;
	}

	void Physics_SetGravity(fvec3* value) {
		Scene* context = ScriptEngine::Get()->GetContext();
		PhysicsSettings physics_settings = context->GetPhysicsSettings();
		physics_settings.gravity = *value;
		context->SetPhysicsSettings(physics_settings);
	}

	void Entity_GetEntity(MonoString* name, UUID* out_id) 
	{
		Scene* context = ScriptEngine::Get()->GetContext();
		std::string_view str_view(mono_string_to_utf8(name));
		Entity e = context->GetEntity(str_view);
		mono_free((void*)str_view.data());
		
		*out_id = e.GetComponent<UUIDComponent>();
	}

	void ScriptComponent_GetLayer(uint64 entity_id, int32* out_layer) {
		Scene* context = ScriptEngine::Get()->GetContext();
		Entity entity = context->GetEntity(UUID(entity_id));

		*out_layer = entity.GetComponent<SpriteComponent>().layer;
	}

	void ScriptComponent_SetLayer(uint64 entity_id, int32* layer) {
		Scene* context = ScriptEngine::Get()->GetContext();
		Entity entity = context->GetEntity(UUID(entity_id));

		entity.GetComponent<SpriteComponent>().layer = *layer;
	}

	void ScriptComponent_GetTint(uint64 entity_id, fvec4* out_tint) {
		Scene* context = ScriptEngine::Get()->GetContext();
		Entity entity = context->GetEntity(UUID(entity_id));

		*out_tint = entity.GetComponent<SpriteComponent>().color;
	}

	void ScriptComponent_SetTint(uint64 entity_id, fvec4* tint) {
		Scene* context = ScriptEngine::Get()->GetContext();
		Entity entity = context->GetEntity(UUID(entity_id));

		entity.GetComponent<SpriteComponent>().color = *tint;
	}

	glm::vec2 Vector2MultiplyByScalar(glm::vec2 vector, float32 scalar) {
		return vector * scalar;
	}

	glm::vec3 Vector3MultiplyByScalar(glm::vec3 vector, float32 scalar) {
		return vector * scalar;
	}

	glm::vec4 Vector4MultiplyByScalar(glm::vec4 vector, float32 scalar) {
		return vector * scalar;
	}

	glm::vec2 Vector2Lerp(glm::vec2 v1, glm::vec2 v2, float32 factor) {
		return v1 * (1 - factor) + v2 * factor;
	}

	glm::vec2 Vector3Lerp(glm::vec3 v1, glm::vec3 v2, float32 factor) {
		return v1 * (1 - factor) + v2 * factor;
	}

	glm::vec2 Vector4Lerp(glm::vec4 v1, glm::vec4 v2, float32 factor) {
		return v1 * (1 - factor) + v2 * factor;
	}

	glm::quat BuildQuatFromEulerAngles(glm::vec3 angles) {
		return glm::quat(angles);
	}

	glm::quat QuatRotate(glm::quat quat, float32 angle, glm::vec3 axis) {
		return glm::rotate(quat, angle, axis);
	}

	glm::quat QuatSlerp(glm::quat x, glm::quat y, float32 factor) {
		return glm::slerp(x, y, factor);
	}

	glm::quat QuatNormalize(glm::quat x) {
		return glm::normalize(x);
	}

	glm::vec3 QuatToEulerAngles(glm::quat quat) {
		return glm::eulerAngles(quat);
	}

	glm::quat QuatInverse(glm::quat quat) {
		return glm::inverse(quat);
	}

	void AudioComponent_PlayInlined(MonoString* path) {
		char* data = mono_string_to_utf8(path);
		AudioEngine* audio_engine = AudioEngine::Get();
		audio_engine->StartPlaybackInlined(data);
		mono_free(data);
	}

	void ScriptAPI::AddInternalCalls()
	{
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Input_KeyPressed);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Input_GetDeltaTime);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Logger_Log);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(TagComponent_GetTag);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(TransformComponent_GetTranslation);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(TransformComponent_SetTranslation);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(TransformComponent_GetRotation);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(TransformComponent_SetRotation);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(TransformComponent_GetScale);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(TransformComponent_SetScale);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(ScriptComponent_GetLayer);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(ScriptComponent_SetLayer);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(ScriptComponent_GetTint);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(ScriptComponent_SetTint);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(RigidBody2D_AddLinearImpulse);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(RigidBody2D_GetLinearVelocity);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(RigidBody2D_SetLinearVelocity);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(RigidBody2D_AddForce);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(RigidBody2D_AddTorque);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Physics_GetGravity);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Physics_SetGravity);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Entity_GetEntity);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Vector2MultiplyByScalar);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Vector3MultiplyByScalar);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Vector4MultiplyByScalar);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Vector3Lerp);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Vector2Lerp);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Vector4Lerp);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(BuildQuatFromEulerAngles);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(QuatToEulerAngles);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(QuatRotate);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(QuatSlerp);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(QuatNormalize);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(QuatInverse);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(AudioComponent_PlayInlined);
	}

}

