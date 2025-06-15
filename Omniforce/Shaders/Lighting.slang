module Lighting;

import Common;
import Gen.RenderingGenerated;

namespace Omni {

public extension Omni.PointLight {
    public float ComputeAttenuation(float3 WorldPosition) {
        float DistanceSquared = distance(Position, WorldPosition);
        DistanceSquared = max(DistanceSquared, 1e-4f);

        return Intensity / DistanceSquared;
    }
    }
}

public float DistributionGGX(float3 N, float3 H, float Roughness)
{
    float a = Roughness * Roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
public float GeometrySchlickGGX(float NdotV, float Roughness)
{
    float R = (Roughness + 1.0);
    float K = (R * R) / 8.0;

    float Nom = NdotV;
    float Denom = NdotV * (1.0 - K) + K;

    return Nom / Denom;
}
// ----------------------------------------------------------------------------
public float GeometrySmith(float3 N, float3 V, float3 L, float Roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float GGX2 = GeometrySchlickGGX(NdotV, Roughness);
    float GGX1 = GeometrySchlickGGX(NdotL, Roughness);

    return GGX1 * GGX2;
}
// ----------------------------------------------------------------------------
public float3 FresnelSchlick(float CosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - CosTheta, 0.0, 1.0), 5.0);
}

}