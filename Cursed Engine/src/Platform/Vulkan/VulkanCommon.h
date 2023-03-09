#pragma once

#include <Log/Logger.h>
#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <vulkan/vulkan.h>
#include "ForwardDecl.h"

#ifdef CURSED_DEBUG
	#define VK_CHECK_RESULT(fn) if(fn != VK_SUCCESS) CURSED_CORE_ERROR("Vulkan call failed: {0} ({1})", __FILE__, __LINE__);
#else
	#define VK_CHECK_RESULT(fn) fn
#endif