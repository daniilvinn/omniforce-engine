#include "AppWindow.h"

#include <Platform/Win64/Win64AppWindow.h>

namespace Omni {

	

	Shared<AppWindow> AppWindow::Create(const Config& config)
	{
		switch (OMNIFORCE_PLATFORM)
		{
		case OMNIFORCE_PLATFORM_WIN64:		return std::make_shared<Win64AppWindow>(config);	break;
		default:							return nullptr;										break;
		}
	}

}