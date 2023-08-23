#pragma once

#include "MonoForwardDecl.h"

#include <Core/UUID.h>
#include <Foundation/Macros.h>
#include <Foundation/Types.h>



namespace Omni {

	class ScriptClass;

	class OMNIFORCE_API RuntimeScriptInstance {
	public:
		RuntimeScriptInstance(ScriptClass* script_class, UUID id);
		~RuntimeScriptInstance();

		MonoObject* Raw() const { return mManagedObject; }

		void InvokeInit();
		void InvokeUpdate();
		void InvokeMethod(std::string method_name, void** params);

	private:
		MonoObject* mManagedObject;
		ScriptClass* mClass;
		uint32 mGCHandle;
		MonoMethod* mOnInit;
		MonoMethod* mOnUpdate;

		friend class ScriptEngine;
	};

}