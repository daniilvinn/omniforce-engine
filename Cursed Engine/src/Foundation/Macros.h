#pragma once

#ifdef CURSED_STATIC
	#define CURSED_API
#elif defined(CURSED_DYNAMIC)
	#error "Cursed Engine currently doesn't support dynamic linking. Coming soon!"
#endif

#ifdef _DEBUG
	#define CURSED_DEBUG
#elif defined(NDEBUG)
	#define CURSED_RELEASE
#endif // defined _DEBUG

#define BIT(x) (1 << x)

#define CLASS_API public
#define PROTECTED_METHODS protected
#define INTERNAL_UTILS private

#define PUBLIC_DATA public
#define PROTECTED_DATA protected
#define INTERNAL_DATA private

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
