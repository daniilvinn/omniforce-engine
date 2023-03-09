#pragma once

#define CURSED_DEBUG_CONFIG 0x1
#define CURSED_RELEASE_CONFIG 0x2

#ifdef _DEBUG
	#define CURSED_DEBUG
	#define CURSED_BUILD_CONFIG CURSED_DEBUG_CONFIG
#elif defined(NDEBUG)
	#define CURSED_RELEASE
	#define CURSED_BUILD_CONFIG CURSED_RELEASE_CONFIG
#endif // defined _DEBUG

#define BIT(x) (1 << x)

#ifdef _WIN32
	#ifndef _WIN64
		#error Engine requires 64-bit system to build.
	#endif
#else
	#error Only Windows platform is supported (WIP)
#endif

#define CURSED_PLATFORM_WIN64 0x1
#define CURSED_PLATFORM_LINUX 0x2
#define CURSED_PLATFORM_MACOS 0x3

#if defined(_WIN32) || defined(_WIN64)
	#define CURSED_PLATFORM CURSED_PLATFORM_WIN64
#endif

#if CURSED_PLATFORM == CURSED_PLATFORM_WIN64
	#ifdef CURSED_DYNAMIC
		#ifdef CURSED_BUILD_DLL
			#define CURSED_API __declspec(dllexport)
		#else
			#define CURSED_API __declspec(dllimport)
		#endif
	#elif defined(CURSED_STATIC)
		#define CURSED_API
	#endif
#endif