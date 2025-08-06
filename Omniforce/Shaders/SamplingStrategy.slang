#pragma once

module SamplingStrategy;

import Common;
import Sampler;
import Lighting;
import BSDF;
import NextEventEstimation;
import InlineRayTracing;
import MIS;
import Rendering;

using namespace Omni;

namespace Omni {

public struct SamplingResult {
    public float3 Direction;
    public float3 Weight;
    public float PDF;

    static SamplingResult Invalid() {
        SamplingResult Result;
        Result.Weight = 0;
        Result.PDF = 0;
        Result.Direction = FloatMax;

        return Result;
    }
}

// ------------------------------------
// BRDF sampling strategy
// Probabilistically chooses between
// specular and diffuse lobes sampling
// ------------------------------------
public struct BRDFSamplingStrategy
{
    public SamplingResult Sample<SurfaceDomain Domain = SurfaceDomain.Opaque>(
        PBRSurface<Domain> Surface,
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

        float3 F0 = FresnelBaseReflectance<SurfaceDomain.Opaque>(Surface.Color, Surface.Metallic);

        // Early exit for grazing angles
        if (NdotV < FloatMin) {
            Result.Direction = FloatMax;
            return SamplingResult.Invalid();
        }

        // --- 1) Perfect mirror shortcut -------------------------------
        if (Surface.Metallic >= 1.0 && Surface.Roughness < 0.01)
        {
            Result.Direction = normalize(reflect(-V, Surface.TangentSpace.N));
            float NdotL = max(dot(Surface.TangentSpace.N, Result.Direction), 0.0);

            if (NdotL < FloatMin) {
                return SamplingResult.Invalid();
            }

            // Fresnel at view angle
            float3 Fm = FresnelSchlick(NdotV, F0);

            Result.PDF = 1.0;
            Result.Weight = Fm;

            return Result;
        }

        // --- 2) Common precomputations ---------------------------------
        Surface.Roughness = max(Surface.Roughness, 0.001); // avoid singularities

        // Fresnel at view direction for energy distribution
        float3 Fv = FresnelSchlick(NdotV, F0);
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
            return SamplingResult.Invalid();
        }

        // --- 4) Direct BRDF evaluation (optimized path) ---------------
        float NdotL = max(dot(Surface.TangentSpace.N, Result.Direction), 0.0);
        float NdotH = max(dot(Surface.TangentSpace.N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        // Early exit for invalid geometry
        if (NdotL < FloatMin || NdotH < FloatMin || VdotH < FloatMin) {
            return SamplingResult.Invalid();
        }

        // --- 5) Calculate PDF -----------------------------------------
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
            return SamplingResult.Invalid();
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
            return SamplingResult.Invalid();
        }

        return Result;
    }
};

// ------------------------------------
// Next Event Estimation sampling strategy
// Performs direct sampling of light sources
// ------------------------------------
public struct NEESamplingStrategy {
    public SamplingResult Sample<SurfaceDomain Domain = SurfaceDomain.Opaque>(
        PBRSurface<Domain> Surface,
        float3 V_world,
        float2 Xi,
        NextEventEstimator NEE
    ) {
        if (NEE.GetLightCount() == 0) {
            return SamplingResult.Invalid();
        }

        SamplingResult Result;
        Result.Weight = 0;
        Result.PDF = 0;
        Result.Direction = FloatMax;

        // Ensure view direction points toward surface
        float3 V = normalize(V_world);
        float NdotV = max(dot(Surface.TangentSpace.N, V), 0.0);

        float3 F0 = FresnelBaseReflectance<SurfaceDomain.Opaque>(Surface.Color, Surface.Metallic);

        // Early exit for grazing angles
        if (NdotV < FloatMin) {
            return SamplingResult.Invalid();
        }

        // --- 1) Perfect mirror shortcut -------------------------------
        if (Surface.Metallic >= 1.0 && Surface.Roughness < 0.01)
        {
            return SamplingResult.Invalid();
        }

        // --- 2) NEE Sampling ------------------------------------------
        LightSample NEESample = NEE.Sample(Xi, Surface.Position, Surface.TangentSpace.N);

        // Check if sample if valid
        if (!NEESample.Valid) {
            return SamplingResult.Invalid();
        }

        // --- 3) Shadow ray tracing ------------------------------------
        float3 L = normalize(NEESample.Position - Surface.Position);

        RayDesc ShadowRayDesc;
        ShadowRayDesc.Origin = Surface.Position;
        ShadowRayDesc.Direction = L;
        ShadowRayDesc.TMin = 0.001f;
        ShadowRayDesc.TMax = distance(NEESample.Position, Surface.Position) - 0.01f;

        const uint RayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

        InlineRayTracer ShadowRayTracer;
        HitInfo NEEHitInfo = ShadowRayTracer.TraceRay<RayFlags>(ShadowRayDesc);

        if (NEEHitInfo.Hit) {
            return SamplingResult.Invalid();
        }

        // --- 4) Compute BRDF ------------------------------------------
        float3 H = normalize(V + L); 
        float NdotL = dot(Surface.TangentSpace.N, L);
        float VdotH = dot(V, H);

        // Early exit if light is behind the surface
        if (NdotL <= 0.0) {
            return SamplingResult.Invalid();
        }

        float D = DistributionGGX(Surface.TangentSpace.N, H, Surface.Roughness);
        float G = GeometrySmith(Surface.TangentSpace.N, V, L, Surface.Roughness);

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

        // --- 5) Calculate BSDF PDF for MIS --------------------------
        float3 F0_bsdf = FresnelBaseReflectance<SurfaceDomain.Opaque>(Surface.Color, Surface.Metallic);
        float3 Fv_bsdf = F0_bsdf + (1.0 - F0_bsdf) * pow(clamp(1.0 - max(dot(Surface.TangentSpace.N, V), 0.0), 0.0, 1.0), 5.0);
        float3 kS_bsdf = Fv_bsdf;
        float3 kD_bsdf = (1.0 - kS_bsdf) * (1.0 - Surface.Metallic);
        
        float wS_bsdf = dot(kS_bsdf, float3(0.299, 0.587, 0.114));
        float wD_bsdf = dot(kD_bsdf, float3(0.299, 0.587, 0.114));
        float totalWeight_bsdf = wS_bsdf + wD_bsdf + 1e-8;
        float pSpec_bsdf = wS_bsdf / totalWeight_bsdf;
        float pDiff_bsdf = wD_bsdf / totalWeight_bsdf;
        
        float pdfSpec_bsdf = 0.0;
        float pdfDiff_bsdf = 0.0;
        
        if (pSpec_bsdf > FloatMin) {
            float D_bsdf = D;
            pdfSpec_bsdf = D_bsdf * max(dot(Surface.TangentSpace.N, H), 0.0) / (4.0 * VdotH);
        }
        
        if (pDiff_bsdf > FloatMin) {
            pdfDiff_bsdf = NdotL * InvPi;
        }
        
        float BSDFPdf = pSpec_bsdf * pdfSpec_bsdf + pDiff_bsdf * pdfDiff_bsdf;

        // --- 6) Gather results ----------------------------------------
        AreaToSolidAngleJacobian Jacobian;
        float SolidAnglePDF = Jacobian.Convert(Surface.Position, NEESample);

        if (SolidAnglePDF < 1e-8) {
            return SamplingResult.Invalid();
        }

        // Apply MIS weights
        PowerHeuristic MISHeuristic;
        float MISWeight = MISHeuristic.Compute(SolidAnglePDF, BSDFPdf);
        
        float3 LightEmission = NEESample.Color * NEESample.Intensity;
        Result.Weight = f * NdotL * LightEmission * MISWeight / SolidAnglePDF;
        Result.PDF = SolidAnglePDF;
        Result.Direction = L;

        // Check for invalid values
        if (!all(isfinite(Result.Weight))) {
            return SamplingResult.Invalid();
        }

        return Result;
    }
}

public struct BSDFSamplingStrategy {
    public SamplingResult Sample<SurfaceDomain Domain = SurfaceDomain.Transmissive>(
        PBRSurface<Domain> Surface,
        float3 V_world,
        float2 Xi
    ) {
        SamplingResult Result;
        Result.Weight = 0;
        Result.PDF = 0;
        Result.Direction = FloatMax;

        // Ensure view direction points toward surface
        float3 V = normalize(V_world);
        float3 N = SameHemisphere(V, Surface.TangentSpace.N) ? Surface.TangentSpace.N : -Surface.TangentSpace.N;
        float NdotV = max(dot(N, V), 0.0);

        // TODO: This is not correct, we need track correct current medium IOR
        float EtaI;
        float EtaT;

        if(SameHemisphere(V, Surface.TangentSpace.N)) {
            EtaI = 1.0;
            EtaT = Surface.Transmission.IOR;
        } else {
            EtaI = Surface.Transmission.IOR;
            EtaT = 1.0;
        }

        float3 F0 = FresnelBaseReflectance<SurfaceDomain.Transmissive>(
            Surface.Color, 
            Surface.Metallic, 
            EtaI, 
            EtaT
        );


        // Early exit for grazing angles
        if (NdotV < FloatMin) {
            Result.Direction = FloatMax;
            return SamplingResult.Invalid();
        }

        // --- 1) Perfect mirror shortcut -------------------------------
        if (false && Surface.Metallic >= 1.0 && Surface.Roughness < 0.01) // TODO: Remove this
        {
            Result.Direction = normalize(reflect(-V, Surface.TangentSpace.N));
            float NdotL = max(dot(Surface.TangentSpace.N, Result.Direction), 0.0);

            if (NdotL < FloatMin) {
                return SamplingResult.Invalid();
            }

            // Fresnel at view angle
            float3 Fm = FresnelSchlick(NdotV, F0);

            Result.PDF = 1.0;
            Result.Weight = Fm;

            return Result;
        }

        // --- 2) Common precomputations ---------------------------------
        Surface.Roughness = max(Surface.Roughness, 0.001); // avoid singularities

        // Fresnel at view direction for energy distribution
        float3 Fv = FresnelSchlick(NdotV, F0);
        float3 kReflect = Fv;
        
        // Refraction only possible for non-metallic transmissive surfaces
        float3 kRefract = (1.0 - kReflect) * (1.0 - Surface.Metallic);
        
        // For non-transmissive domains, no refraction is possible
        if (Domain != SurfaceDomain.Transmissive) {
            kRefract = 0.0;
        }

        // Use luminance weights for MIS probabilities
        float wReflect = dot(kReflect, float3(0.299, 0.587, 0.114));
        float wRefract = dot(kRefract, float3(0.299, 0.587, 0.114));
        float totalWeight = wReflect + wRefract + 1e-8;
        float pReflect = wReflect / totalWeight;
        float pRefract = wRefract / totalWeight;

        // --- 3) Branch: Reflect vs Refract sampling ------------------
        // For now we only support specular reflection with no roughness
        if (Xi.x < pReflect && pReflect > FloatMin)
        {
            Result.Direction = normalize(reflect(-V, N));
            Result.PDF = pReflect;
            Result.Weight = Fv * Surface.Color;

            return Result;
        }
        else if (pRefract > FloatMin)
        {
            float Eta;
            
            Eta = EtaI / EtaT;

            Result.Direction = normalize(refract(-V, N, Eta));
            Result.PDF = pRefract;
            Result.Weight = (1.0 - Fv) * Surface.Color * (1.0f - Surface.Transmission.Factor);

            return Result;
        }
        else
        {
            return SamplingResult.Invalid();
        }
        
        return Result;
    }
};

}