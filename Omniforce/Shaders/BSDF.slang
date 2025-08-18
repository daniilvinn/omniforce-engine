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

    public float PDF(float3 V, float3 L) {
        float NdotV = max(dot(TBN.N, V), 0.0);
        float NdotL = max(dot(TBN.N, L), 0.0);

        if (NdotV < FloatMin || NdotL < FloatMin) {
            return 0.0;
        }

        float3 H = normalize(V + L);
        float NdotH = max(dot(TBN.N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotH < FloatMin || VdotH < FloatMin) {
            return 0.0;
        }

        // Specular PDF (GGX)
        float D = DistributionGGX(TBN.N, H, Roughness);
        float PDFSpec = D * NdotH / (4.0 * VdotH);

        return PDFSpec;
    }

    public float3 ComputeAndGetPDF(float3 V, float3 L, out float pdf) {
        pdf = PDF(V, L);
        return Compute(V, L);
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
        float NdotV = max(dot(TBN.N, V), 0.0);
        float NdotL = max(dot(TBN.N, -L), 0.0); // Note: L points toward surface for transmission

        if (NdotV < FloatMin || NdotL < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // Compute the microfacet normal (half vector) for transmission
        // For transmission: wm = -normalize(wi + wo/eta)
        float3 H = -normalize(L + V / Eta);
        
        // Ensure half vector points into the same hemisphere as the surface normal
        if (dot(H, TBN.N) < 0.0) {
            H = -H;
        }

        float NdotH = max(dot(TBN.N, H), 0.0);
        float VdotH = dot(V, H);
        float LdotH = dot(L, H);

        if (NdotH < FloatMin || abs(VdotH) < FloatMin || abs(LdotH) < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // GGX Distribution
        float D = DistributionGGX(TBN.N, H, Roughness);

        // Smith Geometry function
        float G = GeometrySmith(TBN.N, V, -L, Roughness);

        // Fresnel term (1 - F for transmission)
        float3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - abs(VdotH), 0.0, 1.0), 5.0);
        float3 T = (1.0 - F); // Transmission coefficient

        // BTDF denominator: ((wi·wm) + (wo·wm)/η)²
        float DenomH = LdotH + VdotH / Eta;
        float DenomHSquared = DenomH * DenomH;

        if (abs(DenomHSquared) < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // BTDF numerator: D * T * G * |wi·wm| * |wo·wm|
        float Numerator = D * G * abs(LdotH) * abs(VdotH);
        
        // Full BTDF denominator: denomHSquared * |cos θi| * |cos θo|
        float FullDenom = DenomHSquared * NdotL * NdotV;

        if (FullDenom < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // Final BTDF value
        float BTDFValue = Numerator / FullDenom;
        
        // Apply transmission color and scale by eta²
        float3 Result = BaseColor * T * BTDFValue * (Eta * Eta);

        return Result;
    }
};

}