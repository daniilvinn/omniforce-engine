#pragma once

module BSDF;

import Common;
import Lighting;
import Rendering;
import Sampler;

using namespace Omni;

namespace Omni {

struct BSDFSample {
    float3 Direction;
    float PDF;
};

// ------------------------------------
// Lambertian Diffuse BRDF
// ------------------------------------
public struct LambertianDiffuseBRDF {
    float3 BaseColor;
    float3 DiffuseWeight;

    public static CosineWeightedHemisphereSampler Sampler;

    public __init(float3 InBaseColor, float3 InDiffuseWeight) {
        BaseColor = InBaseColor;
        DiffuseWeight = InDiffuseWeight;
    }

    public float3 Compute(float3 V, float3 L) {
        // Use pre-computed diffuse weight that includes proper energy conservation
        return DiffuseWeight * BaseColor * InvPi;
    }
}

// ------------------------------------
// Cook-Torrance BRDF Evaluator
// ------------------------------------
public struct CookTorranceBRDF {
    float3 BaseColor;
    float Metallic;
    float Roughness;
    float3 F0;
    TangentSpace TBN;

    public static GGXVNDFSampler Sampler;

    public __init(float3 InBaseColor, float InMetallic, float InRoughness, float3 InF0, TangentSpace InTBN) {
        BaseColor = InBaseColor;
        Metallic = InMetallic;
        Roughness = max(InRoughness, 0.001); // avoid singularities
        F0 = InF0;
        TBN = InTBN;
    }

    public float3 Compute(float3 V, float3 L) {
        float NdotV = max(dot(TBN.N, V), 0.0);
        float NdotL = max(dot(TBN.N, L), 0.0);

        if (NdotV < FloatMin || NdotL < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        float3 H = normalize(V + L);
        float NdotH = max(dot(TBN.N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotH < FloatMin || VdotH < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // GGX Distribution
        float D = DistributionGGX(TBN.N, H, Roughness);

        // Smith Geometry function
        float G = GeometrySmith(TBN.N, V, L, Roughness);

        // Fresnel term
        float3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);

        // Cook-Torrance specular term
        float Denom = 4.0 * NdotV * NdotL + 1e-8;
        float3 Specular = (D * G * F) / Denom;

        return Specular;
    }

};

public struct RoughBTDF {
    float3 BaseColor;
    float Metallic;
    float Roughness;
    float3 F0;
    TangentSpace TBN;
    float Eta;

    public static GGXVNDFSampler Sampler;

    public __init(float3 InBaseColor, float InMetallic, float InRoughness, float3 InF0, TangentSpace InTBN, float InEta) {
        BaseColor = InBaseColor;
        Metallic = InMetallic;
        Roughness = max(InRoughness, 0.001); // avoid singularities
        F0 = InF0;
        TBN = InTBN;
        Eta = InEta;
    }

    public float3 Compute(float3 V, float3 L) {
        // V is the incident direction, L is the transmitted direction
        // Both V and L point away from the surface for transmission
        float NdotV = max(dot(TBN.N, V), 0.0);
        float NdotL = max(dot(TBN.N, -L), 0.0); // L points away from surface for transmission
        
        if (NdotV < FloatMin || NdotL < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // Compute the half-vector for transmission using correct formula
        // For transmission BTDF: H = -normalize(ηi * ωi + ηt * ωt)
        // Since Eta = ηi/ηt, we get: H = -normalize(Eta * V + L)
        float3 H = -normalize(Eta * V + L);
        
        // Ensure half vector points toward the correct hemisphere
        if (dot(H, TBN.N) < 0.0) {
            H = -H;
        }

        float NdotH = max(dot(TBN.N, H), 0.0);
        float VdotH = dot(V, H);
        float LdotH = dot(L, H);

        if (NdotH < FloatMin || abs(VdotH) < FloatMin || abs(LdotH) < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // GGX Distribution: D(ω_m)
        float D = DistributionGGX(TBN.N, H, Roughness);

        // Smith Geometry function: G(ω_i, ω_o) - V=ω_i (incident), -L=ω_o (transmitted)
        float G = GeometrySmith(TBN.N, V, -L, Roughness);

        // Fresnel term: F(ωi · ωm) - using VdotH (incident direction)
        float3 F = FresnelSchlick(VdotH, F0);;

        // BTDF denominator: ((ωi · ωm) + (ωo · ωm)/η)²
        // For our convention: ωi · ωm = VdotH, ωo · ωm = -LdotH (since L points away)
        float DenomH = VdotH - LdotH / Eta;
        float DenomHSquared = DenomH * DenomH;

        if (abs(DenomHSquared) < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // BTDF with proper energy conservation factor η²
        // Numerator: η² * D(ωm) * (1-F(ωi·ωm)) * G(ωi,ωo) * |ωi·ωm| * |ωo·ωm|
        float EtaSquared = Eta * Eta;
        float3 Numerator = EtaSquared * D * (1.0 - F) * G * abs(VdotH) * abs(LdotH);
        
        // Denominator: DenomHSquared * |cosθi| * |cosθo|
        float Denominator = DenomHSquared * NdotV * NdotL;

        if (Denominator < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // Apply the corrected theoretical formula with energy conservation
        float3 Result = BaseColor * (Numerator / Denominator);

        return Result;
    }
};

}