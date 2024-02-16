#pragma once

#include <Log/Logger.h>
#include <Renderer/RendererCommon.h>
#include "ForwardDecl.h"

#include <volk.h>

#ifdef CURSED_DEBUG
	#define VK_CHECK_RESULT(fn)																								\
			{ VkResult res = fn;																							\
			if(res != VK_SUCCESS) {																							\
				CURSED_CORE_ERROR("Vulkan call failed ({0}): {1} ({2})", VkResultToString(res), __FILE__, __LINE__); 		\
				std::unreachable();																							\
			}}
#else
	#define VK_CHECK_RESULT(fn) fn
#endif

constexpr std::string VkResultToString(VkResult result) 
{
	switch (result)
	{
	case VK_SUCCESS:
		return "VK_SUCCESS";
		break;
	case VK_NOT_READY:
		return "VK_NOT_READY";
		break;
	case VK_TIMEOUT:
		return "VK_TIMEOUT";
		break;
	case VK_EVENT_SET:
		return "VK_EVENT_SET";
		break;
	case VK_EVENT_RESET:
		return "VK_EVENT_RESET";
		break;
	case VK_INCOMPLETE:
		return "VK_INCOMPLETE";
		break;
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return "VK_ERROR_OUT_OF_HOST_MEMORY";
		break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		break;
	case VK_ERROR_INITIALIZATION_FAILED:
		return "VK_ERROR_INITIALIZATION_FAILED";
		break;
	case VK_ERROR_DEVICE_LOST:
		return "VK_ERROR_DEVICE_LOST";
		break;
	case VK_ERROR_MEMORY_MAP_FAILED:
		return "VK_ERROR_MEMORY_MAP_FAILED";
		break;
	case VK_ERROR_LAYER_NOT_PRESENT:
		return "VK_ERROR_LAYER_NOT_PRESENT";
		break;
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		return "VK_ERROR_EXTENSION_NOT_PRESENT";
		break;
	case VK_ERROR_FEATURE_NOT_PRESENT:
		return "VK_ERROR_FEATURE_NOT_PRESENT";
		break;
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		return "VK_ERROR_INCOMPATIBLE_DRIVER";
		break;
	case VK_ERROR_TOO_MANY_OBJECTS:
		return "VK_ERROR_TOO_MANY_OBJECTS";
		break;
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		break;
	case VK_ERROR_FRAGMENTED_POOL:
		return "VK_ERROR_FRAGMENTED_POOL";
		break;
	case VK_ERROR_UNKNOWN:
		return "VK_ERROR_UNKNOWN";
		break;
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		return "VK_ERROR_OUT_OF_POOL_MEMORY";
		break;
	case VK_ERROR_INVALID_EXTERNAL_HANDLE:
		return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		break;
	case VK_ERROR_FRAGMENTATION:
		return "VK_ERROR_FRAGMENTATION";
		break;
	case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
		return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		break;
	case VK_PIPELINE_COMPILE_REQUIRED:
		return "VK_PIPELINE_COMPILE_REQUIRED";
		break;
	case VK_ERROR_SURFACE_LOST_KHR:
		return "VK_ERROR_SURFACE_LOST_KHR";
		break;
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		break;
	case VK_SUBOPTIMAL_KHR:
		return "VK_SUBOPTIMAL_KHR";
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
		return "VK_ERROR_OUT_OF_DATE_KHR";
		break;
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		break;
	case VK_ERROR_VALIDATION_FAILED_EXT:
		return "VK_ERROR_VALIDATION_FAILED_EXT";
		break;
	case VK_ERROR_INVALID_SHADER_NV:
		return "VK_ERROR_INVALID_SHADER_NV";
		break;
	case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
		return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		break;
	case VK_ERROR_NOT_PERMITTED_KHR:
		return "VK_ERROR_NOT_PERMITTED_KHR";
		break;
	case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
		return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		break;
	case VK_THREAD_IDLE_KHR:
		return "VK_THREAD_IDLE_KHR";
		break;
	case VK_THREAD_DONE_KHR:
		return "VK_THREAD_DONE_KHR";
		break;
	case VK_OPERATION_DEFERRED_KHR:
		return "VK_OPERATION_DEFERRED_KHR";
		break;
	case VK_OPERATION_NOT_DEFERRED_KHR:
		return "VK_OPERATION_NOT_DEFERRED_KHR";
		break;
	case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
		return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
		break;
	default:
		std::unreachable();
		break;
	}
}