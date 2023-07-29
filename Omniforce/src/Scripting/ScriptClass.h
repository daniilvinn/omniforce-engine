#pragma once

#include "MonoForwardDecl.h"

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Memory/Pointers.hpp>
#include <Core/UUID.h>
#include "RuntimeScriptInstance.h"

#include <fmt/format.h>

#include <string>
#include <string_view>

namespace Omni {

	class OMNIFORCE_API ScriptClass {
	public:
		ScriptClass() = default;
		ScriptClass(const std::string& script_namespace, const std::string& script_name, bool extract_from_core);
		
		MonoClass* Raw() const { return m_Class; }
		MonoMethod* GetMethod(const std::string& name, uint32 param_count);
		std::string_view GetNamespace() const { return m_Namespace; }
		std::string_view GetName() const { return m_Name; }
		std::string GetFullName() const;
		bool IsValid() const { return m_Class != nullptr; }

		Shared<RuntimeScriptInstance> AllocateObject(UUID id);

	private:
		MonoClass* m_Class;
		std::string m_Namespace;
		std::string m_Name;

		friend class ScriptEngine;
	};

}