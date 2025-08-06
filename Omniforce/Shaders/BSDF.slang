#pragma once

module BSDF;

import Common;
import Lighting;

namespace Omni {

// ------------------------------------
// Cook-Torrance BRDF Evaluator
// ------------------------------------
public struct CookTorranceBRDF {
    float3 BaseColor;
    float Metallic;
    float Roughness;
    float3 F0;
    float3 N, T, B; // Surface normal and tangent frame

    [mutating]
    public void Initialize(float3 InBaseColor, float InMetallic, float InRoughness, float3 InF0, float3 InNormal, float3 InTangent, float3 InBitangent) {
        BaseColor = InBaseColor;
        Metallic = InMetallic;
        Roughness = max(InRoughness, 0.001); // avoid singularities
        F0 = InF0;
        N = InNormal;
        T = InTangent;
        B = InBitangent;
    }

    public float3 Compute(float3 V, float3 L) {
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);

        if (NdotV < FloatMin || NdotL < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        float3 H = normalize(V + L);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotH < FloatMin || VdotH < FloatMin) {
            return float3(0.0, 0.0, 0.0);
        }

        // GGX Distribution
        float D = DistributionGGX(N, H, Roughness);

        // Smith Geometry function
        float G = GeometrySmith(N, V, L, Roughness);

        // Fresnel term
        float3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);

        // Cook-Torrance specular term
        float denom = 4.0 * NdotV * NdotL + 1e-8;
        float3 Specular = (D * G * F) / denom;

        // Lambertian diffuse term
        float3 kD = (1.0 - F) * (1.0 - Metallic);
        float3 Diffuse = kD * BaseColor * InvPi;

        return Diffuse + Specular;
    }

    public float PDF(float3 V, float3 L) {
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);

        if (NdotV < FloatMin || NdotL < FloatMin) {
            return 0.0;
        }

        float3 H = normalize(V + L);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotH < FloatMin || VdotH < FloatMin) {
            return 0.0;
        }

        // Calculate MIS weights
        float3 Fv = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
        float3 kS = Fv;
        float3 kD = (1.0 - kS) * (1.0 - Metallic);

        float wS = dot(kS, float3(0.299, 0.587, 0.114));
        float wD = dot(kD, float3(0.299, 0.587, 0.114));
        float totalWeight = wS + wD + 1e-8;
        float pSpec = wS / totalWeight;
        float pDiff = wD / totalWeight;

        // Specular PDF (GGX VNDF)
        float pdfSpec = 0.0;
        if (pSpec > FloatMin) {
            float D = DistributionGGX(N, H, Roughness);
            pdfSpec = D * NdotH / (4.0 * VdotH);
        }

        // Diffuse PDF (cosine-weighted)
        float pdfDiff = 0.0;
        if (pDiff > FloatMin) {
            pdfDiff = NdotL * InvPi;
        }

        return pSpec * pdfSpec + pDiff * pdfDiff;
    }

    public float3 ComputeAndGetPDF(float3 V, float3 L, out float pdf) {
        pdf = PDF(V, L);
        return Compute(V, L);
    }
};

}