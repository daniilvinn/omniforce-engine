#include "Input.h"

#include <Platform/Win64/Win64Input.h>
#include <Log/Logger.h>

namespace Cursed {

	Input* Input::s_Instance;

	void Input::Init()
	{
		switch (CURSED_PLATFORM)
		{
		case CURSED_PLATFORM_WIN64:
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
		CURSED_CORE_INFO("Input system shutdown success");
	}

}