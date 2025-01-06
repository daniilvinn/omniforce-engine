#include <Foundation/Common.h>
#include <Core/Windowing/AppWindow.h>

#include <Platform/Win64/Win64AppWindow.h>

namespace Omni {

	Ref<AppWindow> AppWindow::Create(IAllocator* allocator, const Config& config)
	{
		switch (OMNIFORCE_PLATFORM)
		{
		case OMNIFORCE_PLATFORM_WIN64:		return CreateRef<Win64AppWindow>(allocator, config);
		default:							return nullptr;										
		}
	}

}