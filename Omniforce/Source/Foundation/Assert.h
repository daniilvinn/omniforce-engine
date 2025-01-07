#pragma once

#include <Foundation/Platform.h>
#include <Foundation/Log/Logger.h>

#ifdef OMNIFORCE_DEBUG
	#define OMNIFORCE_ASSERT(expression)																	\
					do {																					\
						if(!(expression)) {																	\
							OMNIFORCE_CORE_ERROR("Assertion failed: {0}({1})", __FILE__, __LINE__);			\
							__debugbreak();																	\
						}																					\
					} while (false)
	#define OMNIFORCE_ASSERT_TAGGED(expression, ...)														\
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