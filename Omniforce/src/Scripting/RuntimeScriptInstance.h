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

		void InvokeInit();
		void InvokeUpdate();		

	private:
		MonoObject* mManagedObject;
		uint32 mGCHandle;
		MonoMethod* mOnInit;
		MonoMethod* mOnUpdate;

		friend class ScriptEngine;
	};

}