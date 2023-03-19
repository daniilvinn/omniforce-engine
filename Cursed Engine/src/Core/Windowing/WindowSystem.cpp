#include "WindowSystem.h"
#include <Platform/Win64/Win64WindowSystem.h>

namespace Cursed {

	WindowSystem* WindowSystem::s_Instance = nullptr;

	WindowSystem* WindowSystem::Init()
	{
		switch (CURSED_PLATFORM)
		{
		case CURSED_PLATFORM_WIN64:
			s_Instance = new Win64WindowSystem();
			return s_Instance;
		default:
			break;
		}
	}

}