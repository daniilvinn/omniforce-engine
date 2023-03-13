#include "AppWindow.h"

#include <Platform/Win64/Win64AppWindow.h>

namespace Cursed {

	

	Shared<AppWindow> AppWindow::Create(const Config& config)
	{
		switch (CURSED_PLATFORM)
		{
		case CURSED_PLATFORM_WIN64:
			return std::make_shared<Win64AppWindow>(config);
			//return Shared<Win64AppWindow>::Create(config).Cast<AppWindow>();
			break;
		default:
			break;
		}
	}

}