#include <Foundation/Common.h>
#include <Core/Windowing/WindowSystem.h>

#include <Platform/Win64/Win64WindowSystem.h>

namespace Omni {

	Ptr<WindowSystem> WindowSystem::s_Instance;

	Ptr<WindowSystem>& WindowSystem::Init()
	{
		switch (OMNIFORCE_PLATFORM)
		{
		case OMNIFORCE_PLATFORM_WIN64:
			s_Instance = CreatePtr<Win64WindowSystem>(&g_PersistentAllocator);
			return s_Instance;
		default:
			std::unreachable();
		}
	}

}