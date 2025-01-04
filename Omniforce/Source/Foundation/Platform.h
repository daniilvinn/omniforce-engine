#pragma once

#define OMNIFORCE_DEBUG_CONFIG 0x1
#define OMNIFORCE_RELEASE_CONFIG 0x2
#define OMNIFORCE_RELWITHDEBINFO_CONFIG 0x3

#ifdef OMNIFORCE_DEBUG
	#define OMNIFORCE_BUILD_CONFIG OMNIFORCE_DEBUG_CONFIG
#elif defined(OMNIFORCE_RELEASE)
	#define OMNIFORCE_BUILD_CONFIG OMNIFORCE_RELEASE_CONFIG
#elif defined(OMNIFORCE_RELWITHDEBINFO)
	#define OMNIFORCE_BUILD_CONFIG OMNIFORCE_RELWITHDEBINFO_CONFIG
#endif // defined OMNIFORCE_DEBUG

#define BIT(x) (1 << x)

#ifdef _WIN32
	#ifndef _WIN64
		#error Engine requires 64-bit system to build.
	#endif
#else
	#error Only Windows platform is supported
#endif

#define OMNIFORCE_PLATFORM_WIN64 0x1
#define OMNIFORCE_PLATFORM_LINUX 0x2
#define OMNIFORCE_PLATFORM_MACOS 0x3

#if defined(_WIN32) || defined(_WIN64)
	#define OMNIFORCE_PLATFORM OMNIFORCE_PLATFORM_WIN64
#endif

#if OMNIFORCE_PLATFORM == OMNIFORCE_PLATFORM_WIN64
	#ifdef OMNIFORCE_DYNAMIC
		#ifdef OMNIFORCE_BUILD_DLL
			#define OMNIFORCE_API __declspec(dllexport)
		#else
			#define OMNIFORCE_API __declspec(dllimport)
		#endif
	#elif defined(OMNIFORCE_STATIC)
		#define OMNIFORCE_API
	#endif
#endif

#ifndef OMNIFORCE_RELEASE
	#define	OMNI_DEBUG_ONLY_CODE(code) code 
#else
	#define OMNI_DEBUG_ONLY_CODE(code)
#endif

#define OMNI_DEBUG_ONLY_FIELD(field) OMNI_DEBUG_ONLY_CODE(field)