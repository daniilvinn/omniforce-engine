#pragma once

module Lighting;

import Common;
import Gen.RenderingGenerated;

namespace Omni {

public extension Omni.PointLight {
    public float ComputeAttenuation(float3 WorldPosition) {
        float Distance = distance(Position, WorldPosition);
        float DistanceSquared = Distance * Distance;
        DistanceSquared = max(DistanceSquared, 1e-4f);

        return 1.0f / DistanceSquared;
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

// Utility function to compute the base reflectance for a given IOR and metallic value
// For transmissive surfaces, we need proper IOR-based Fresnel calculation
// For non-transmissive surfaces, we use the standard metallic reflectance
public float3 FresnelBaseReflectance<SurfaceDomain Domain>(float3 Color, float Metallic, float IORi = 1, float IORt = 1) {
    if (Domain == SurfaceDomain.Transmissive) {
        return pow((IORi - IORt) / (IORi + IORt), 2.0);
    } else {
        return lerp(0.04f, Color, Metallic);
    }
}

// ----------------------------------------------------------------------------
public float3 FresnelSchlick(float CosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - CosTheta, 0.0, 1.0), 5.0);
}

public struct LightSample {
    public float3 Position;
    public float3 Normal;
    public float3 LightPosition;
    public float3 Color;
    public float Intensity;
    public float PDF;
    public bool Valid;
}

public struct SurfaceTransmissionProperties {
    public float IOR;
    public float Factor;
    public float Thickness = 0.01f;

    static public SurfaceTransmissionProperties Default() {
        return SurfaceTransmissionProperties(1.0, 1.0, 1.0);
    }
}

public struct PBRSurface<SurfaceDomain Domain = SurfaceDomain.Opaque> {
    public TangentSpace TangentSpace;
    public float3 Position;
    public float3 Color;
    public float Metallic;
    public float Roughness;
    private Conditional<SurfaceTransmissionProperties, Domain == SurfaceDomain.Transmissive> TransmittanceProperties;

    public property SurfaceTransmissionProperties Transmission {
        get {
            if (Domain == SurfaceDomain.Transmissive) {
                return TransmittanceProperties.get().value;
            } else {
                return SurfaceTransmissionProperties.Default();
            }
        }
        set {
            if (Domain == SurfaceDomain.Transmissive) {
                TransmittanceProperties.set(newValue);
            }
        }
    }
}

public struct AreaToSolidAngleJacobian {
    public float Convert(float3 P, LightSample Sample) {
        float3 Wi = Sample.Position - P;
        float DistanceSquared = dot(Wi, Wi);
        float3 WiDir = normalize(Wi);

        float CosTheta = dot(Sample.Normal, -WiDir);

        // Avoid division by zero or backfacing surfaces
        if (CosTheta <= 0.0f)
            return 0.0f;

        return Sample.PDF * DistanceSquared / CosTheta;

    }

};

}