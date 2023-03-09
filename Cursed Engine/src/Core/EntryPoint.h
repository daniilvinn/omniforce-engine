#pragma once

#if (CURSED_PLATFORM == CURSED_PLATFORM_WIN64)
	#include <Windows.h>
#endif

extern Cursed::Subsystem* ConstructRootSystem();

using namespace Cursed;

bool g_EngineRunning;

#ifdef CURSED_DEBUG
int main()
#else CURSED_RELEASE
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
#endif // defined CURSED_DEBUG
{
	g_EngineRunning = true;

	while (g_EngineRunning) 
	{
		Scope<Application> app = std::make_unique<Application>();
		
		Application::Options options;
		options.root_system = ConstructRootSystem();
		options.flags = 0;

		app->Launch(options);
		app->Run();
		app->Destroy();

		g_EngineRunning = false;
	}
}
