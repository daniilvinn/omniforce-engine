#include "../RuntimeScriptInstance.h"

#include "../ScriptEngine.h"

#include <mono/jit/jit.h>

namespace Omni {

	RuntimeScriptInstance::RuntimeScriptInstance(ScriptClass* script_class, UUID id)
		: mClass(script_class)
	{
		ScriptEngine* script_engine = ScriptEngine::Get();
		mManagedObject = mono_object_new(ScriptEngine::Get()->GetDomain(), script_class->Raw());
		mGCHandle = mono_gchandle_new(mManagedObject, true);
		
		void* parameter = &id;
		MonoObject* exception = nullptr;
		mono_runtime_invoke(script_engine->mBaseCtor, mManagedObject, &parameter, &exception);

		mOnInit = mono_object_get_virtual_method(mManagedObject, script_engine->mInitMethodBase);
		mOnUpdate = mono_object_get_virtual_method(mManagedObject, script_engine->mUpdateMethodBase);
	}

	RuntimeScriptInstance::~RuntimeScriptInstance()
	{
		mono_gchandle_free(mGCHandle);
	}

	void RuntimeScriptInstance::InvokeInit()
	{
		MonoObject* exc;
		mono_runtime_invoke(mOnInit, mManagedObject, nullptr, &exc);
	}

	void RuntimeScriptInstance::InvokeUpdate()
	{
		MonoObject* exc;
		mono_runtime_invoke(mOnUpdate, mManagedObject, nullptr, &exc);
	}

	void RuntimeScriptInstance::InvokeMethod(std::string method_name, void** params)
	{
		MonoObject* exc = nullptr;

		MonoMethod* method = mClass->GetMethod(method_name, 0);
		if (!method)
			return;

		mono_runtime_invoke(method, mManagedObject, params, & exc);

		if (exc)
			ScriptEngine::Get()->DispatchExceptionHandling(exc);
	}

}