module Common;

import Gen.BasicTypes;

namespace Omni {

public static const float PI = 3.141592653589;
public static const float NaN = 0.0 / 0.0;
public static const float InvPi = 0.31830988618379067154;
public static const float Inv2Pi = 0.15915494309189533577;
public static const float Inv4Pi = 0.07957747154594766788;
public static const float PiOver2 = 1.57079632679489661923;
public static const float PiOver4 = 0.78539816339744830961;
public static const float Sqrt2 = 1.41421356237309504880;

[[vk::binding(0, 0)]]
public Sampler2D TextureHeap[];

[[vk::binding(1, 0)]]
public RWTexture2D<uint64_t> SceneVisibilityBuffer;

[[vk::binding(2, 0)]]
public RaytracingAccelerationStructure SceneTLAS;

[[vk::binding(3, 0)]]
public RWTexture2D<float4> OutputImage;

[[vk::binding(4, 0)]]
public RWTexture2D<float4> History;

public float3 QuaternionRotate(float4 Quat, float3 Point) {
    float3 uv = cross(Quat.xyz, Point);
    float3 uuv = cross(Quat.xyz, uv);
    return Point + ((uv * Quat.w) + uuv) * 2.0;
}

public extension Omni.Transform {
    public float3 TransformPoint(float3 Point) {
        return QuaternionRotate(rotation, Point * scale) + translation;
    }
}

public extension Omni.Frustum {
    public bool IntersectSphere(Sphere InSphere) {
        bool result = true;
        for (int i = 0; i < 6; i++) {
            Plane plane = planes[i];
            result = result && ((dot(plane.normal, InSphere.center) - plane.distance) > -InSphere.radius);
        }
        return result;
    }
}

public float2 VerticalInvert(float2 x) {

    float2 Result = x;
    Result.y = 1.0f - Result.y;

    return Result;
}

public float3 VerticalInvert(float3 x) {

    float3 Result = x;
    Result.y = 1.0f - Result.y;

    return Result;
}

public float4 VerticalInvert(float4 x) {

    float4 Result = x;
    Result.y = 1.0f - Result.y;

    return Result;
}

public float2 Invert(float2 x) {
    return 1.0f - x;
}

public float3 Invert(float3 x) {
    return 1.0f - x;
}

public float4 Invert(float4 x) {
    return 1.0f - x;
}

}