#include "ScriptAPI.h"

#include <Core/Input/Input.h>
#include <Core/UUID.h>
#include <Log/Logger.h>

#include <Scene/Component.h>

#include <Scripting/ScriptEngine.h>

#include <mono/jit/jit.h>

#define OMNI_REGISTER_SCRIPT_API_FUNCTION(name) mono_add_internal_call("Omni.EngineAPI::" #name, name)

namespace Omni {

	glm::vec3 Transform_GetTranslation(uint64 entity_id) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		return entity.GetComponent<TRSComponent>().translation;
	}

	void Transform_SetTranslation(uint64 entity_id, glm::vec3 translation) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		entity.GetComponent<TRSComponent>().translation = translation;
	}

	glm::vec3 Transform_GetScale(uint64 entity_id) 
	{
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		return entity.GetComponent<TRSComponent>().scale;
	}

	void Transform_SetScale(uint64 entity_id, glm::vec3 scale) {
		ScriptEngine* script_engine = ScriptEngine::Get();
		Scene* context = script_engine->GetContext();
		Entity entity(context->GetEntities()[entity_id], context);
		entity.GetComponent<TRSComponent>().scale = scale;
	}

	bool Input_KeyPressed(KeyCode code) {
		return Input::KeyPressed(code);
	}

	void Logger_Log(Logger::Level severity, MonoString* message) {
		char* msg = mono_string_to_utf8(message);
		switch (severity)
		{
		case Logger::Level::LEVEL_TRACE:			OMNIFORCE_CLIENT_TRACE("[Script] {}", msg);		break;
		case Logger::Level::LEVEL_INFO:				OMNIFORCE_CLIENT_INFO("[Script] {}", msg);		break;
		case Logger::Level::LEVEL_WARN:				OMNIFORCE_CLIENT_WARNING("[Script] {}", msg);	break;
		case Logger::Level::LEVEL_ERROR:			OMNIFORCE_CLIENT_ERROR("[Script] {}", msg);		break;
		case Logger::Level::LEVEL_CRITICAL:			OMNIFORCE_CLIENT_CRITICAL("[Script] {}", msg);	break;
		case Logger::Level::LEVEL_NONE:																break;
		default:									std::unreachable();								break;
		}
		mono_free(msg);
	}

	void ScriptAPI::AddInternalCalls()
	{
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Input_KeyPressed);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Logger_Log);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Transform_GetTranslation);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Transform_SetTranslation);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Transform_GetScale);
		OMNI_REGISTER_SCRIPT_API_FUNCTION(Transform_SetScale);
	}

}

