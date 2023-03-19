#include "Input.h"

#include <Platform/Win64/Win64Input.h>
#include <Log/Logger.h>

namespace Omni {

	Input* Input::s_Instance;

	void Input::Init()
	{
		switch (OMNIFORCE_PLATFORM)
		{
		case OMNIFORCE_PLATFORM_WIN64:
			s_Instance = new Win64Input;
			break;
		default:
			std::unreachable();
			break;
		}
	}

	void Input::Shutdown()
	{
		delete s_Instance;
		OMNIFORCE_CORE_INFO("Input system shutdown success");
	}

}