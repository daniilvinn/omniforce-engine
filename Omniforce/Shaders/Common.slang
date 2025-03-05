module Common;

import Gen.BasicTypes;

namespace Omni {

[[vk::binding(0, 0)]]
public Sampler2D TextureHeap[65535];

[[vk::binding(1, 0)]]
RWTexture2D<uint64_t> SceneVisibilityBuffer;

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

}