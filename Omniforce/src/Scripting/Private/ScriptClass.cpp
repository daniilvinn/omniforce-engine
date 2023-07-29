#include "../ScriptClass.h"

#include "../ScriptEngine.h"

#include <mono/jit/jit.h>

namespace Omni {

	ScriptClass::ScriptClass(const std::string& script_namespace, const std::string& script_name, bool extract_from_core)
		: m_Namespace(script_namespace), m_Name(script_name)
	{
		ScriptEngine* script_engine = ScriptEngine::Get();
		MonoImage* image = extract_from_core ? script_engine->GetCoreImage() : script_engine->GetAppImage();
		m_Class = mono_class_from_name(image, script_namespace.c_str(), script_name.c_str());
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& name, uint32 param_count)
	{
		return mono_class_get_method_from_name(m_Class, name.c_str(), param_count);
	}

	std::string ScriptClass::GetFullName() const
	{
		std::string full_name = fmt::format("{}.{}", m_Namespace, m_Name);
		if (!m_Namespace.length())
			full_name.erase(0, 1);
		return full_name;
	}

	Shared<RuntimeScriptInstance> ScriptClass::AllocateObject(UUID id)
	{
		return std::make_shared<RuntimeScriptInstance>(this, id);
	}

}