#include "WindowSystem.h"
#include <Platform/Win64/Win64WindowSystem.h>

namespace Omni {

	WindowSystem* WindowSystem::s_Instance = nullptr;

	WindowSystem* WindowSystem::Init()
	{
		switch (OMNIFORCE_PLATFORM)
		{
		case OMNIFORCE_PLATFORM_WIN64:
			s_Instance = new Win64WindowSystem();
			return s_Instance;
		default:
			break;
		}
	}

}