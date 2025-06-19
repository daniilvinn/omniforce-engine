#pragma once

module Sampler;

import Common;
import Lighting;

namespace Omni {

    // ------------------------------------
    // Concentric Disk Sampler
    // ------------------------------------
    public struct ConcentricDiskSampler {
        float2 Sample(float2 u) {
            float2 uOffset = 2.f * u - float2(1, 1);

            if (uOffset.x == 0 && uOffset.y == 0)
                return float2(0, 0);

            float theta, r;
            if (abs(uOffset.x) > abs(uOffset.y)) {
                r = uOffset.x;
                theta = PiOver4 * (uOffset.y / uOffset.x);
            } else {
                r = uOffset.y;
                theta = PiOver2 - PiOver4 * (uOffset.x / uOffset.y);
            }
            return r * float2(cos(theta), sin(theta));
        }
    }

    // ------------------------------------
    // Uniform Hemisphere Sampler
    // ------------------------------------
    public struct UniformHemisphereSampler {
        public float3 Sample(float2 U) {
            float phi = 2.0f * PI * U.x;
            float r = sqrt(U.y);
            float x = r * cos(phi);
            float y = r * sin(phi);
            float z = sqrt(1.0f - U.y);
            return float3(x, y, z);
        }
    }

    // ------------------------------------
    // Cosine-weighted Hemisphere Sampler
    // ------------------------------------
    public struct CosineWeightedHemisphereSampler {
        public float3 Sample(float2 u) {
            float2 d = ConcentricDiskSampler().Sample(u);
            float z = sqrt(max(0.0, 1.0 - d.x * d.x - d.y * d.y));
            return float3(d.x, d.y, z);
        }
    }

    // ------------------------------------
    // GGX VNDF Sampler (Heitz 2018)
    // ------------------------------------
    public struct GGXVNDFSampler {
        public float3 Sample(float3 ViewDir, float Roughness, float2 Xi)
        {
            float alpha = Roughness * Roughness;

            // Normalize view direction (should point toward surface)
            float3 V = normalize(ViewDir);

            // Transform view to hemisphere configuration
            float3 Vh = normalize(float3(alpha * V.x, alpha * V.y, V.z));

            float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
            float3 T1 = (lensq > 0.0)
               ? normalize(float3(-Vh.y, Vh.x, 0.0))
               : float3(1.0, 0.0, 0.0);
            float3 T2 = cross(Vh, T1);

            float r = sqrt(Xi.x);
            float phi = 2.0 * PI * Xi.y;
            float t1 = r * cos(phi);
            float t2 = r * sin(phi);

            float s = 0.5 * (1.0 + Vh.z);
            t2 = (1.0 - s) * sqrt(max(0.0, 1.0 - t1 * t1)) + s * t2;

            float t3 = sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2));

            float3 Nh = t1 * T1 + t2 * T2 + t3 * Vh;

            // Transform back to world space
            float3 H = normalize(float3(alpha * Nh.x, alpha * Nh.y, max(0.0, Nh.z)));
            return H;
        }
    };

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

    // ------------------------------------
    // Main sampling strategy.
    // Uses MIS for combining BSDF sampling and
    // cosine-weighted hemisphere sampling.
    // ------------------------------------
    public struct PathTracingSamplingStrategy
    {
        public void Sample(
            TangentSpace TBN,
            float3 V_world,
            float3 BaseColor, 
            float Metallic,
            float RoughnessIn, 
            float3 F0,
            float2 Xi,
            out float3 L, 
            out float3 weight, 
            out float pdf
        ) {
            // Initialize outputs
            L = FloatMax;
            weight = float3(0, 0, 0);
            pdf = 0.0;

            // Ensure view direction points toward surface
            float3 V = normalize(V_world);
            float NdotV = max(dot(TBN.N, V), 0.0);

            // Early exit for grazing angles
            if (NdotV < FloatMin) {
                L = FloatMax;
                return;
            }

            // --- 1) Perfect mirror shortcut -------------------------------
            if (Metallic >= 1.0 && RoughnessIn < 0.01)
            {
                L = normalize(reflect(-V, TBN.N));
                float NdotL = max(dot(TBN.N, L), 0.0);

                if (NdotL < FloatMin) {
                    return;
                }

                // Fresnel at view angle
                float3 Fm = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);

                pdf = 1.0;
                weight = Fm;
                return;
            }

            // --- 2) Common precomputations ---------------------------------
            float Roughness = max(RoughnessIn, 0.001); // avoid singularities

            // Fresnel at view direction for energy distribution
            float3 Fv = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);
            float3 kS = Fv;
            float3 kD = (1.0 - kS) * (1.0 - Metallic); // energy conserving diffuse

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
                float3 V_local = float3(dot(V, TBN.T), dot(V, TBN.B), dot(V, TBN.N));

                // Sample microfacet normal H in local space
                GGXVNDFSampler vndf;
                float3 H_local = vndf.Sample(V_local, Roughness, xi_spec);
                H = normalize(H_local.x * TBN.T + H_local.y * TBN.B + H_local.z * TBN.N);

                // Reflect view around H to get outgoing direction L
                L = normalize(reflect(-V, H));
            }
            else if (pDiff > FloatMin)
            {
                // ---- Diffuse (cosine) sampling ----
                float2 xi_diff = float2((Xi.x - pSpec) / pDiff, Xi.y);

                CosineWeightedHemisphereSampler cosSamp;
                float3 L_local = cosSamp.Sample(xi_diff);

                L = normalize(L_local.x * TBN.T + L_local.y * TBN.B + L_local.z * TBN.N);
                H = normalize(V + L);
            }
            else
            {
                L = FloatMax;
                return;
            }

            // --- 4) Direct BRDF evaluation (optimized path) ---------------
            float NdotL = max(dot(TBN.N, L), 0.0);
            float NdotH = max(dot(TBN.N, H), 0.0);
            float VdotH = max(dot(V, H), 0.0);
            float LdotH = max(dot(L, H), 0.0);

            // Early exit for invalid geometry
            if (NdotL < FloatMin || NdotH < FloatMin || VdotH < FloatMin) {
                L = FloatMax;
                return;
            }

            // --- 5) Calculate PDF for MIS (reuse from original) -----------
            float pdfSpec = 0.0;
            float pdfDiff = 0.0;

            if (pSpec > FloatMin) {
                float D = DistributionGGX(TBN.N, H, Roughness);
                pdfSpec = D * NdotH / (4.0 * VdotH);
            }

            if (pDiff > FloatMin) {
                pdfDiff = NdotL * InvPi;
            }

            pdf = pSpec * pdfSpec + pDiff * pdfDiff;

            if (pdf < 1e-8) {
                L = FloatMax;
                return;
            }

            // --- 6) Final BRDF evaluation (reuse calculations) -----------
            float D = DistributionGGX(TBN.N, H, Roughness);
            float G = GeometrySmith(TBN.N, V, L, Roughness);

            // Fresnel at half-vector H
            float3 Fh = F0 + (1.0 - F0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);

            // Cook-Torrance specular term
            float denom = 4.0 * NdotV * NdotL + 1e-8;
            float3 Specular = (D * G * Fh) / denom;

            // Lambertian diffuse term
            float3 kD_final = (1.0 - Fh) * (1.0 - Metallic);
            float3 Diffuse = kD_final * BaseColor * InvPi;

            // Total BRDF
            float3 f = Diffuse + Specular;

            // Importance sampling weight: f * cos(theta) / pdf
            weight = f * NdotL / pdf;

            // Check for invalid values
            if (!isfinite(weight.x) || !isfinite(weight.y) || !isfinite(weight.z)) {
                weight = float3(0.0, 0.0, 0.0);
                pdf = 0.0;
                L = FloatMax;
            }
        }
    };

} // namespace Omni
