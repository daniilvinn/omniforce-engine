#pragma once

#if __OMNI_COMPILE_HOST_CODE
	#include <Shaders/Slang/Shared/TypeAlises.h>
    using glm::dot;
	#define SHARED_MODULE_EXPORT
#else
	module RenderingCommon;
	#define SHARED_MODULE_EXPORT public
#endif

namespace Omni {

    SHARED_MODULE_EXPORT struct Transform {
        float3 translation;
        half4 rotation;
        float3 scale;

        #if !__OMNI_COMPILE_HOST_CODE
        SHARED_MODULE_EXPORT float3 RotateByQuat(float3 vertex, float4 quat) {
            float3 uv = cross(quat.xyz, vertex);
            float3 uuv = cross(quat.xyz, uv);
            return vertex + ((uv * quat.w) + uuv) * 2.0;
        };

        SHARED_MODULE_EXPORT half3 RotateByQuat(half3 vertex, half4 quat) {
            half3 uv = cross(quat.xyz, vertex);
            half3 uuv = cross(quat.xyz, uv);
            return vertex + ((uv * quat.w) + uuv) * 2.0;
        };

        SHARED_MODULE_EXPORT float3 Apply(float3 point, Transform transform) {
            return RotateByQuat(point * transform.scale, float4(transform.rotation)) + transform.translation;
        }
        #endif
    };

    SHARED_MODULE_EXPORT struct ShaderTexture {
        uint texture_index;
        uint uv_channel_index;
    };

}

#if !__OMNI_COMPILE_HOST_CODE
#undef SHARED_MODULE_EXPORT
#endif