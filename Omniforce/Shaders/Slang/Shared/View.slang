#pragma once

#if __OMNI_COMPILE_HOST_CODE
	#include <Shaders/Slang/Shared/TypeAlises.h>
#endif

#if !__OMNI_COMPILE_HOST_CODE
module View;

import CoreTypes;
#endif

namespace Omni {

struct CameraData {
    float4x4 view;
    float4x4 proj;
    float4x4 view_proj;
    float3 position;
    // Frustum frustum;
    float fov; // in radians
    float viewport_height;
    float viewport_width;
    float near_clip_distance;
    float far_clip_distance;
}

}