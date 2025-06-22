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
public static const float FloatMax = 3.402823466e+38F;
public static const float FloatMin = 1.175494351e-38F;

[[vk::binding(0, 0)]]
public Sampler2D TextureHeap[];

[[vk::binding(1, 0)]]
public RWTexture2D<uint64_t> SceneVisibilityBuffer;

[[vk::binding(2, 0)]]
public RaytracingAccelerationStructure SceneTLAS;

[[vk::binding(3, 0)]]
public RWTexture2D<float4> OutputImage;

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

public T Invert<T>(T x) where T : IFloat {
    return T(1.0f) - x;
}

public bool IsInvalidHitPoint<T>(T x) where T : IFloat {
    return any(x == T(FloatMax));
}

public bool SameHemisphere(float3 a, float3 b, float3 N)
{
    return (dot(N, a) > 0.0) && (dot(N, b) > 0.0);
}

// Normal pull up method to fix normals for path tracing
// https://github.com/Devsh-Graphics-Programming/Nabla/blob/master/include/nbl/builtin/glsl/bxdf/common.glsl
public struct NormalFixer {
    internal bool IsNormalValid(float3 N, float3 V, float3 G, out float VdotN, out float NdotG, out float VdotG, out float firstReflectionPart)
    {
        VdotN = dot(V, N);
        NdotG = dot(N, G);
        VdotG = dot(V, G);

        firstReflectionPart = 2.f * VdotN * NdotG;

        return (sign(VdotG) * firstReflectionPart) >= abs(VdotG);
    }

    internal bool IsNormalValid(float3 N, float3 V, float3 G, out float VdotN, out float NdotG, out float VdotG)
    {
        float dummy;
        return IsNormalValid(N, V, G, VdotN, NdotG, VdotG, dummy);
    }

    public float3 PullNormal(float3 N, float3 V, float3 G)
    {
        float VdotN, NdotG, VdotG;
        if (IsNormalValid(N, V, G, VdotN, NdotG, VdotG))
            return N;

        float3 R = N * VdotG - V * NdotG;
        float lenR2 = dot(R, R);
        
        float3 newN = normalize(R + V * sqrt(lenR2));

        NdotG = dot(newN, G);
        return (NdotG < 0.f) ? (-newN) : newN;
    }
};

public struct BarycentricDerivative
{
    float3 Lambda;
    float3 ddx;
    float3 ddy;

    // Compute barycentric coordinates of a pixel between 3 vertices
    // Also analytically compute derivates
    // Vertices are in clip-space, just after MVP transformation
    [mutating]
    public void Setup(float4 v0, float4 v1, float4 v2, float2 PixelNDC, float2 ImageSize)
    {
        float3 inverse_w = 1.0 / float3(v0.w, v1.w, v2.w);

        float2 ndc0 = v0.xy * inverse_w.x;
        float2 ndc1 = v1.xy * inverse_w.y;
        float2 ndc2 = v2.xy * inverse_w.z;

        float inverse_determinant = 1.0 / determinant(float2x2(ndc2 - ndc1, ndc0 - ndc1));

        ddx = float3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y) * inverse_determinant * inverse_w;
        ddy = float3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x) * inverse_determinant * inverse_w;

        float ddx_sum = dot(ddx, float3(1.0));
        float ddy_sum = dot(ddy, float3(1.0));

        float2 delta_vector = PixelNDC - ndc0;
        float interpolated_inverse_w = inverse_w.x + delta_vector.x * ddx_sum + delta_vector.y * ddy_sum;
        float interpolated_w = 1.0 / interpolated_inverse_w;

        Lambda.x = interpolated_w * (inverse_w[0] + delta_vector.x * ddx.x + delta_vector.y * ddy.x);
        Lambda.y = interpolated_w * (0.0 + delta_vector.x * ddx.y + delta_vector.y * ddy.y);
        Lambda.z = interpolated_w * (0.0 + delta_vector.x * ddx.z + delta_vector.y * ddy.z);

        ddx *= (2.0 / ImageSize.x);
        ddy *= (2.0 / ImageSize.y);
        ddx_sum *= (2.0 / ImageSize.x);
        ddy_sum *= (2.0 / ImageSize.y);

        ddy *= -1.0;
        ddy_sum *= -1.0;

        float interpolated_w_ddx = 1.0 / (interpolated_inverse_w + ddx_sum);
        float interpolated_w_ddy = 1.0 / (interpolated_inverse_w + ddy_sum);

        ddx = interpolated_w_ddx * (Lambda * interpolated_inverse_w + ddx) - Lambda;
        ddy = interpolated_w_ddy * (Lambda * interpolated_inverse_w + ddy) - Lambda;
    }

    // Scalar float interpolation utils
    public float Interpolate(float v0, float v1, float v2)
    {
        float3 MergedVertex = float3(v0, v1, v2);

        return dot(MergedVertex, Lambda);
    }

    public float Interpolate(float v0, float v1, float v2, out float OutDdx, out float OutDdy)
    {
        float3 MergedVertex = float3(v0, v1, v2);

        OutDdx = dot(MergedVertex, ddx);
        OutDdy = dot(MergedVertex, ddy);

        return dot(MergedVertex, Lambda);
    }

    // float2 interpolation utils
    public float2 Interpolate(float2 v0, float2 v1, float2 v2)
    {
        float2 MergedVertex = v0 * Lambda.x + v1 * Lambda.y + v2 * Lambda.z;

        return MergedVertex;
    }

    public float2 Interpolate(float2 v0, float2 v1, float2 v2, out float2 OutDdx, out float2 OutDdy)
    {
        float2 MergedVertex = v0 * Lambda.x + v1 * Lambda.y + v2 * Lambda.z;

        OutDdx = v0 * ddx.x + v1 * ddx.y + v2 * ddx.z;
        OutDdy = v0 * ddy.x + v1 * ddy.y + v2 * ddy.z;

        return MergedVertex;
    }

    // float3 interpolation utils
    public float3 Interpolate(float3 v0, float3 v1, float3 v2)
    {
        float3 MergedVertex = v0 * Lambda.x + v1 * Lambda.y + v2 * Lambda.z;

        return MergedVertex;
    }

    public float3 Interpolate(float3 v0, float3 v1, float3 v2, out float3 OutDdx, out float3 OutDdy)
    {
        float3 MergedVertex = v0 * Lambda.x + v1 * Lambda.y + v2 * Lambda.z;

        OutDdx = v0 * ddx.x + v1 * ddx.y + v2 * ddx.z;
        OutDdy = v0 * ddy.x + v1 * ddy.y + v2 * ddy.z;

        return MergedVertex;
    }

    // float4 interpolation utils
    public float4 Interpolate(float4 v0, float4 v1, float4 v2)
    {
        float4 MergedVertex = v0 * Lambda.x + v1 * Lambda.y + v2 * Lambda.z;

        return MergedVertex;
    }

    public float4 Interpolate(float4 v0, float4 v1, float4 v2, out float4 OutDdx, out float4 OutDdy)
    {
        float4 MergedVertex = v0 * Lambda.x + v1 * Lambda.y + v2 * Lambda.z;

        OutDdx = v0 * ddx.x + v1 * ddx.y + v2 * ddx.z;
        OutDdy = v0 * ddy.x + v1 * ddy.y + v2 * ddy.z;

        return MergedVertex;
    }

};

public struct TangentSpace {
    public float3 T, B, N;

    public __init() {}
    public __init(float3 InNormal) {
        float3 UpVector = float3(0.0f, 1.0f, 0.0f);
        float3 ReferenceVector = abs(dot(UpVector, InNormal)) > 0.9 ? float3(1.0f, 0.0f, 0.0f) : UpVector;

        T = normalize(cross(InNormal, ReferenceVector));
        B = cross(InNormal, T);
    }

    public __init(float3 InTangent, float3 InBitangent, float3 InNormal) {
        T = InTangent;
        B = InBitangent;
        N = InNormal;
    }

    public property float3x3 TBN {
        public get {
            return float3x3(T, B, N);
        }
        private set {}
    }
    
}


}