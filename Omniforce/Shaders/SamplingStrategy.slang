#pragma once

module SamplingStrategy;

import Common;
import Sampler;
import Lighting;
import BSDF;

namespace Omni {

public struct SamplingResult {
    public float3 Direction;
    public float3 Weight;
    public float PDF;
}

// ------------------------------------
// BSDF sampling strategy
// Probabilistically chooses between
// specular and diffuse lobes sampling
// ------------------------------------
public struct BSDFSamplingStrategy
{
    public SamplingResult Sample(
        PBRSurface Surface,
        float3 V_world,
        float2 Xi
    ) {
        SamplingResult Result;
        Result.Weight = 0;
        Result.PDF = 0;
        Result.Direction = FloatMax;

        // Ensure view direction points toward surface
        float3 V = normalize(V_world);
        float NdotV = max(dot(Surface.TangentSpace.N, V), 0.0);

        float3 F0 = lerp(0.04f, Surface.Color, Surface.Metallic);

        // Early exit for grazing angles
        if (NdotV < FloatMin) {
            Result.Direction = FloatMax;
            return Result;
        }

        // --- 1) Perfect mirror shortcut -------------------------------
        if (Surface.Metallic >= 1.0 && Surface.Roughness < 0.01)
        {
            Result.Direction = normalize(reflect(-V, Surface.TangentSpace.N));
            float NdotL = max(dot(Surface.TangentSpace.N, Result.Direction), 0.0);

            if (NdotL < FloatMin) {
                return Result;
            }

            // Fresnel at view angle
            float3 Fm = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);

            Result.PDF = 1.0;
            Result.Weight = Fm;
            return Result;
        }

        // --- 2) Common precomputations ---------------------------------
        Surface.Roughness = max(Surface.Roughness, 0.001); // avoid singularities

        // Fresnel at view direction for energy distribution
        float3 Fv = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
        float3 kS = Fv;
        float3 kD = (1.0 - kS) * (1.0 - Surface.Metallic); // energy conserving diffuse

        // Use luminance weights for MIS probabilities
        float wS = dot(kS, float3(0.299, 0.587, 0.114));
        float wD = dot(kD, float3(0.299, 0.587, 0.114));
        float totalWeight = wS + wD + 1e-8;
        float pSpec = wS / totalWeight;
        float pDiff = wD / totalWeight;

        // --- 3) Branch: Specular vs Diffuse sampling ------------------
        float3 H;

        if (Xi.x < pSpec && pSpec > FloatMin)
        {
            // ---- Specular (GGX VNDF) sampling ----
            float2 xi_spec = float2(Xi.x / pSpec, Xi.y);

            // Transform view to local tangent space
            float3 V_local = float3(
                dot(V, Surface.TangentSpace.T), 
                dot(V, Surface.TangentSpace.B), 
                dot(V, Surface.TangentSpace.N)
            );

            // Sample microfacet normal H in local space
            GGXVNDFSampler vndf;
            float3 H_local = vndf.Sample(V_local, Surface.Roughness, xi_spec);
            H = normalize(mul(H_local, Surface.TangentSpace.TBN));

            // Reflect view around H to get outgoing direction L
            Result.Direction = normalize(reflect(-V, H));
        }
        else if (pDiff > FloatMin)
        {
            // ---- Diffuse (cosine) sampling ----
            float2 xi_diff = float2((Xi.x - pSpec) / pDiff, Xi.y);

            CosineWeightedHemisphereSampler cosSamp;
            float3 L_local = cosSamp.Sample(xi_diff);

            Result.Direction = normalize(mul(L_local, Surface.TangentSpace.TBN));
            H = normalize(V + Result.Direction);
        }
        else
        {
            Result.Direction = FloatMax;
            return Result;
        }

        // --- 4) Direct BRDF evaluation (optimized path) ---------------
        float NdotL = max(dot(Surface.TangentSpace.N, Result.Direction), 0.0);
        float NdotH = max(dot(Surface.TangentSpace.N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        float LdotH = max(dot(Result.Direction, H), 0.0);

        // Early exit for invalid geometry
        if (NdotL < FloatMin || NdotH < FloatMin || VdotH < FloatMin) {
            Result.Direction = FloatMax;
            return Result;
        }

        // --- 5) Calculate PDF for MIS (reuse from original) -----------
        float pdfSpec = 0.0;
        float pdfDiff = 0.0;

        if (pSpec > FloatMin) {
            float D = DistributionGGX(Surface.TangentSpace.N, H, Surface.Roughness);
            pdfSpec = D * NdotH / (4.0 * VdotH);
        }

        if (pDiff > FloatMin) {
            pdfDiff = NdotL * InvPi;
        }

        Result.PDF = pSpec * pdfSpec + pDiff * pdfDiff;

        if (Result.PDF < 1e-8) {
            Result.Direction = FloatMax;
            return Result;
        }

        // --- 6) Final BRDF evaluation (reuse calculations) -----------
        float D = DistributionGGX(Surface.TangentSpace.N, H, Surface.Roughness);
        float G = GeometrySmith(Surface.TangentSpace.N, V, Result.Direction, Surface.Roughness);

        // Fresnel at half-vector H
        float3 Fh = F0 + (1.0 - F0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);

        // Cook-Torrance specular term
        float denom = 4.0 * NdotV * NdotL + 1e-8;
        float3 Specular = (D * G * Fh) / denom;

        // Lambertian diffuse term
        float3 kD_final = (1.0 - Fh) * (1.0 - Surface.Metallic);
        float3 Diffuse = kD_final * Surface.Color * InvPi;

        // Total BRDF
        float3 f = Diffuse + Specular;

        // Importance sampling weight: f * cos(theta) / pdf
        Result.Weight = f * NdotL / Result.PDF;

        // Check for invalid values
        if (!all(isfinite(Result.Weight))) {
            Result.Weight = float3(0.0, 0.0, 0.0);
            Result.PDF = 0.0;
            Result.Direction = FloatMax;
        }

        return Result;
    }
};

// ------------------------------------
// Next Event Estimation sampling strategy
// Performs direct sampling of light sources
// ------------------------------------
public struct NEESamplingStrategy {
    public SamplingResult Sample(
        TangentSpace TBN,
        float3 V_world,
        float3 BaseColor, 
        float Metallic,
        float Roughness, 
        float3 F0,
        float2 Xi
    ) {

    }
}

}