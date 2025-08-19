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
        float3 V,
        float2 Xi
    ) {
        SamplingResult Result;
        Result.Weight = 0;
        Result.PDF = 0;
        Result.Direction = FloatMax;

        // Ensure view direction points toward surface
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
            float3 FresnelFactor = FresnelSchlick(NdotV, F0);

            Result.PDF = 1.0;
            Result.Weight = FresnelFactor;

            return Result;
        }

        // --- 2) Common precomputations ---------------------------------
        Surface.Roughness = max(Surface.Roughness, 0.001); // avoid singularities

        // Fresnel at view direction for energy distribution
        float3 FresnelFactor = FresnelSchlick(NdotV, F0);
        float3 kSpecular = FresnelFactor;
        float3 kDiffuse = (1.0 - kSpecular) * (1.0 - Surface.Metallic); // energy conserving diffuse

        // Use luminance weights for sampling probabilities
        float SpecularWeight = dot(kSpecular, float3(0.299, 0.587, 0.114));
        float DiffuseWeight = dot(kDiffuse, float3(0.299, 0.587, 0.114));
        float TotalWeight = SpecularWeight + DiffuseWeight + 1e-8;
        float SpecularPathProbability = SpecularWeight / TotalWeight;
        float DiffusePathProbability = DiffuseWeight / TotalWeight;

        // --- 3) Branch: Specular vs Diffuse sampling ------------------
        float3 H;

        float PDFDiffuse = 0;
        float PDFSpecular = 0;

        if (Xi.x < SpecularPathProbability && SpecularPathProbability > FloatMin)
        {
            // ---- Specular (GGX VNDF) sampling ----
            float2 XiSpecular = float2(Xi.x / SpecularPathProbability, Xi.y);

            // Transform view to local tangent space
            float3 V_local = float3(
                dot(V, Surface.TangentSpace.T), 
                dot(V, Surface.TangentSpace.B), 
                dot(V, Surface.TangentSpace.N)
            );

            // Sample microfacet normal H in local space using static sampler
            float3 H_local = CookTorranceBRDF.Sampler.Sample(V_local, Surface.Roughness, XiSpecular);
            H = normalize(mul(H_local, Surface.TangentSpace.TBN));

            float3 L_local = normalize(reflect(-V_local, H_local));
            
            // Reflect view around H to get outgoing direction L
            Result.Direction = normalize(reflect(-V, H));

            PDFDiffuse = LambertianDiffuseBRDF.Sampler.PDF(L_local);
            PDFSpecular = CookTorranceBRDF.Sampler.PDF(V, Result.Direction, Surface.TangentSpace, Surface.Roughness);
        }
        else if (DiffusePathProbability > FloatMin)
        {
            // ---- Diffuse (cosine) sampling ----
            float2 XiDiffuse = float2((Xi.x - SpecularPathProbability) / DiffusePathProbability, Xi.y);

            // Use static sampler from LambertianDiffuseBRDF
            float3 L_local = LambertianDiffuseBRDF.Sampler.Sample(XiDiffuse);

            Result.Direction = normalize(mul(L_local, Surface.TangentSpace.TBN));
            H = normalize(V + Result.Direction);

            PDFDiffuse = LambertianDiffuseBRDF.Sampler.PDF(L_local);
            PDFSpecular = CookTorranceBRDF.Sampler.PDF(V, Result.Direction, Surface.TangentSpace, Surface.Roughness);
        }
        else
        {
            return SamplingResult.Invalid();
        }

        // --- 4) Common precomputations ---------------
        float NdotL = max(dot(Surface.TangentSpace.N, Result.Direction), 0.0);
        float NdotH = max(dot(Surface.TangentSpace.N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        // Early exit for invalid geometry
        if (NdotL < FloatMin || NdotH < FloatMin || VdotH < FloatMin) {
            return SamplingResult.Invalid();
        }

        // --- 5) Use separated BRDFs for evaluation -----
        CookTorranceBRDF SpecularBRDF = CookTorranceBRDF(
            Surface.Color,
            Surface.Metallic,
            Surface.Roughness,
            F0,
            Surface.TangentSpace
        );

        LambertianDiffuseBRDF DiffuseBRDF = LambertianDiffuseBRDF(
            Surface.Color,
            (1.0 - FresnelFactor) * (1.0 - Surface.Metallic)
        );

        float3 Specular = SpecularBRDF.Compute(V, Result.Direction);
        float3 Diffuse = DiffuseBRDF.Compute(V, Result.Direction);
        
        float3 f = Specular + Diffuse;

        // Compute final PDF
        Result.PDF = SpecularPathProbability * PDFSpecular + DiffusePathProbability * PDFDiffuse;

        if (Result.PDF < 1e-8) {
            return SamplingResult.Invalid();
        }

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
        float3 V,
        float2 Xi,
        NextEventEstimator NEE,
        MISHeuristic MISHeuristic
    ) {
        if (NEE.GetLightCount() == 0) {
            return SamplingResult.Invalid();
        }

        SamplingResult Result;
        Result.Weight = 0;
        Result.PDF = 0;
        Result.Direction = FloatMax;

        // Ensure view direction points toward surface
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

        // --- 4) Compute BRDF using CookTorranceBRDF ------------------
        float NdotL = dot(Surface.TangentSpace.N, L);

        // Early exit if light is behind the surface
        if (NdotL <= 0.0) {
            return SamplingResult.Invalid();
        }

        // Create separated BRDF evaluators and compute BRDF
        float3 H = normalize(V + L);
        float VdotH = max(dot(V, H), 0.0);
        
        // Compute Fresnel at half vector for proper energy conservation
        float3 FresnelH = FresnelSchlick(VdotH, F0);

        CookTorranceBRDF SpecularBRDF = CookTorranceBRDF(
            Surface.Color,
            Surface.Metallic,
            Surface.Roughness,
            F0,
            Surface.TangentSpace
        );

        LambertianDiffuseBRDF DiffuseBRDF = LambertianDiffuseBRDF(
            Surface.Color,
            (1.0 - FresnelH) * (1.0 - Surface.Metallic)
        );

        float3 Specular = SpecularBRDF.Compute(V, L);
        float3 Diffuse = DiffuseBRDF.Compute(V, L);
        float3 f = Specular + Diffuse;

        // --- 5) Calculate BSDF PDFs using separated BRDFs ---
        float3 L_local = float3(
            dot(L, Surface.TangentSpace.T),
            dot(L, Surface.TangentSpace.B),
            dot(L, Surface.TangentSpace.N)
        );

        float PDFSpecular = SpecularBRDF.Sampler.PDF(V, L, Surface.TangentSpace, Surface.Roughness);
        float PDFDiffuse = DiffuseBRDF.Sampler.PDF(L_local); 

        // Use view-direction Fresnel for MIS probabilities (consistent with BRDFSamplingStrategy)
        float3 FresnelFactor = FresnelSchlick(NdotV, F0);
        float3 kSpecular = FresnelFactor;
        float3 kDiffuse = (1.0 - kSpecular) * (1.0 - Surface.Metallic);
        
        float SpecularWeight = dot(kSpecular, float3(0.299, 0.587, 0.114));
        float DiffuseWeight = dot(kDiffuse, float3(0.299, 0.587, 0.114));
        float TotalWeight = SpecularWeight + DiffuseWeight + 1e-8;
        float SpecularPathProbability = SpecularWeight / TotalWeight;
        float DiffusePathProbability = DiffuseWeight / TotalWeight;

        float BSDFPdf = SpecularPathProbability * PDFSpecular + DiffusePathProbability * PDFDiffuse;

        // --- 6) Gather results ----------------------------------------
        AreaToSolidAngleJacobian Jacobian;
        float SolidAnglePDF = Jacobian.Convert(Surface.Position, NEESample);

        if (SolidAnglePDF < 1e-8) {
            return SamplingResult.Invalid();
        }

        // Apply MIS weights
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
        float3 V,
        float2 Xi
    ) {
        SamplingResult Result;
        Result.Weight = 0;
        Result.PDF = 0;
        Result.Direction = FloatMax;

        // Fix normal if needed
        float3 N = SameHemisphere(V, Surface.TangentSpace.N) ? Surface.TangentSpace.N : -Surface.TangentSpace.N;
        float NdotV = max(dot(N, V), 0.0);

        // TODO: This is not correct (?), we need track correct current medium IOR
        float EtaI;
        float EtaT;

        if(SameHemisphere(V, Surface.TangentSpace.N)) {
            EtaI = 1.0;
            EtaT = Surface.Transmission.IOR;
        } else {
            EtaI = Surface.Transmission.IOR;
            EtaT = 1.0;
        }

        float Eta = EtaI / EtaT;

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
        float3 kRefract = (1.0 - kReflect) * (1.0 - Surface.Metallic); // Refraction only possible for non-metallic transmissive surfaces
        
        // Use luminance weights for MIS probabilities
        float ReflectionWeight = dot(kReflect, float3(0.299, 0.587, 0.114));
        float RefractionWeight = dot(kRefract, float3(0.299, 0.587, 0.114));
        float TotalWeight = ReflectionWeight + RefractionWeight + 1e-8;
        float ReflectionPathProbability = ReflectionWeight / TotalWeight;
        float RefractionPathProbability = RefractionWeight / TotalWeight;

        bool ReflectionChosen = Xi.x < ReflectionPathProbability && ReflectionPathProbability > FloatMin;
        bool RefractionChosen = Xi.x > ReflectionPathProbability && RefractionPathProbability > FloatMin;

        // --- 3) Perfect transmission or reflection shortcut ------------------
        if (false &&Surface.Roughness <= 0.001) {
            if (ReflectionChosen)
            {
                Result.Direction = normalize(reflect(-V, N));
                Result.PDF = ReflectionPathProbability;
                Result.Weight = Fv * Surface.Color;

                return Result;
            }
            else if (RefractionChosen)
            {
                Result.Direction = normalize(refract(-V, N, Eta));
                Result.PDF = RefractionPathProbability;
                Result.Weight = (1.0 - Fv) * Surface.Color * (1.0f - Surface.Transmission.Factor);

                return Result;
            }
            else
            {
                return SamplingResult.Invalid();
            }
        }

        // --- 4) Rough transmission / reflection sampling ----------------------------
        float3 H = RoughBTDF.Sampler.Sample(V, Surface.Roughness, Xi);
        Result.PDF = RoughBTDF.Sampler.PDF(V, normalize(reflect(-V, H)), Surface.TangentSpace, Surface.Roughness);

        float3 f;

        if(ReflectionChosen)
        {
            CookTorranceBRDF SpecularBRDF = CookTorranceBRDF(
                Surface.Color,
                Surface.Metallic,
                Surface.Roughness,
                F0,
                Surface.TangentSpace
            );

            Result.Direction = normalize(reflect(-V, N));

            f = SpecularBRDF.Compute(V, Result.Direction);
        }
        else if(RefractionChosen)
        {
            RoughBTDF RoughBTDF = RoughBTDF(
                Surface.Color,
                Surface.Metallic,
                Surface.Roughness,
                F0,
                Surface.TangentSpace,
                Eta
            );

            Result.Direction = normalize(refract(-V, N, Eta));

            f = RoughBTDF.Compute(V, Result.Direction);
        } 
        else {
            return SamplingResult.Invalid();
        }

        // --- 5) Compute final weight and PDF ----------------------------
        Result.Weight = f * dot(N, Result.Direction) / Result.PDF;

        return Result;
    }
};

}