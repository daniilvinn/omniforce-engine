#pragma once

#define OMNIFORCE_DEBUG_CONFIG 0x1
#define OMNIFORCE_RELEASE_CONFIG 0x2

#ifdef _DEBUG
	#define OMNIFORCE_DEBUG
	#define OMNIFORCE_BUILD_CONFIG OMNIFORCE_DEBUG_CONFIG
#elif defined(NDEBUG)
	#define OMNIFORCE_RELEASE
	#define OMNIFORCE_BUILD_CONFIG OMNIFORCE_RELEASE_CONFIG
#endif // defined _DEBUG

#define BIT(x) (1 << x)

#ifdef _WIN32
	#ifndef _WIN64
		#error Engine requires 64-bit system to build.
	#endif
#else
	#error Only Windows platform is supported (WIP)
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

#ifdef OMNIFORCE_RELEASE
	#define OMNIFORCE_ASSERT(expression)														\
		do {																					\
			if(!(expression)) {																	\
				OMNIFORCE_CORE_ERROR("Assertion failed: {0}({1})", __FILE__, __LINE__);			\
				__debugbreak();																	\
			}																					\
		} while (false)
	#define OMNIFORCE_ASSERT_TAGGED(expression, ...)											\
		do {																					\
			if(!(expression)) {																	\
				OMNIFORCE_CORE_ERROR(															\
					"Assertion failed: {0}({1}) | {2}",											\
					__FILE__,																	\
					__LINE__,																	\
					__VA_ARGS__																	\
				);																				\
				__debugbreak();																	\
			}																					\
		} while (false)
#else 
	#define OMNIFORCE_ASSERT(expression)
	#define OMNIFORCE_ASSERT_TAGGED(expression, ...)
#endif