#pragma once

module Sampler;

import Common;
import Lighting;

namespace Omni {

    // ------------------------------------
    // Concentric Disk Sampler
    // ------------------------------------
    public struct ConcentricDiskSampler {
        public float2 Sample(float2 Xi) {
            float2 U = 2.f * Xi - float2(1, 1);

            if (U.x == 0 && U.y == 0)
                return float2(0, 0);

            float Theta, R;
            if (abs(U.x) > abs(U.y)) {
                R = U.x;
                Theta = PiOver4 * (U.y / U.x);
            } else {
                R = U.y;
                Theta = PiOver2 - PiOver4 * (U.x / U.y);
            }
            return R * float2(cos(Theta), sin(Theta));
        }
        
        public float PDF(float2 sample) {
            // Uniform sampling on unit disk: PDF = 1/π
            float r_squared = dot(sample, sample);
            if (r_squared > 1.0f) return 0.0f; // Outside unit disk
            return 1.0f / PI;
        }
    }

    // ------------------------------------
    // Uniform Sphere Sampler
    // ------------------------------------
    public struct UniformSphereSampler {
        public float3 Sample(float2 Xi)
        {
            // Map Xi.y to z = cos(theta), where theta ∈ [0, π]
            float z = 1.0f - 2.0f * Xi.y;

            // Radius of the circle at height z
            float R = sqrt(max(0.0f, 1.0f - z * z));

            // Map Xi.x to azimuthal angle phi ∈ [0, 2π]
            float Phi = 2.0f * PI * Xi.x;

            // Convert spherical coordinates to Cartesian
            float x = R * cos(Phi);
            float y = R * sin(Phi);

            return float3(x, y, z);
        }
        
        public float PDF(float3 sample) {
            // Uniform sampling on unit sphere: PDF = 1/(4π)
            float len_squared = dot(sample, sample);
            if (abs(len_squared - 1.0f) > 1e-6f) return 0.0f; // Not on unit sphere
            return 1.0f / (4.0f * PI);
        }
    }

    // ------------------------------------
    // Uniform Hemisphere Sampler
    // ------------------------------------
    public struct UniformHemisphereSampler {
        public float3 Sample(float2 Xi) {
            float Phi = 2.0f * PI * Xi.x;
            float CosTheta = Xi.y;
            float SinTheta = sqrt(1.0f - CosTheta * CosTheta);
            float x = SinTheta * cos(Phi);
            float y = SinTheta * sin(Phi);
            float z = CosTheta;
            return float3(x, y, z);
        }
        
        public float PDF(float3 sample) {
            // Uniform sampling on unit hemisphere: PDF = 1/(2π)
            float len_squared = dot(sample, sample);
            if (abs(len_squared - 1.0f) > 1e-6f || sample.z < 0.0f) return 0.0f; // Not on upper hemisphere
            return 1.0f / (2.0f * PI);
        }
    }

    // ------------------------------------
    // Cosine-weighted Hemisphere Sampler
    // ------------------------------------
    public struct CosineWeightedHemisphereSampler {
        public float3 Sample(float2 Xi) {
            float2 d = ConcentricDiskSampler().Sample(Xi);
            float Z = sqrt(max(0.0, 1.0 - d.x * d.x - d.y * d.y));
            return float3(d.x, d.y, Z);
        }
        
        public float PDF(float3 Sample) {
            // Cosine-weighted hemisphere sampling: PDF = cos(θ)/π = sample.z/π
            return Sample.z * InvPi;
        }
    }

    // ------------------------------------
    // GGX VNDF Sampler (Heitz 2018)
    // ------------------------------------
    public struct GGXVNDFSampler {
        public float3 Sample(float3 V, float Roughness, float2 Xi)
        {
            float Alpha = Roughness * Roughness;

            // Transform view to hemisphere configuration
            float3 Vh = normalize(float3(Alpha * V.x, Alpha * V.y, V.z));

            float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
            float3 T1 = (lensq > 0.0)
               ? normalize(float3(-Vh.y, Vh.x, 0.0))
               : float3(1.0, 0.0, 0.0);
            float3 T2 = cross(Vh, T1);

            float R = sqrt(Xi.x);
            float Phi = 2.0 * PI * Xi.y;
            float t1 = R * cos(Phi);
            float t2 = R * sin(Phi);

            float s = 0.5 * (1.0 + Vh.z);
            t2 = (1.0 - s) * sqrt(max(0.0, 1.0 - t1 * t1)) + s * t2;

            float t3 = sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2));

            float3 Nh = t1 * T1 + t2 * T2 + t3 * Vh;

            // Transform back to world space
            float3 H = normalize(float3(Alpha * Nh.x, Alpha * Nh.y, max(0.0, Nh.z)));
            return H;
        }
        
        public float PDF(float3 V, float3 L, float Roughness) {
            float NdotV = max(V.z, 0.0f);
            float NdotL = max(L.z, 0.0f);

            if (NdotV < FloatMin || NdotL < FloatMin) {
                return 0.0f;
            }

            float3 H = normalize(V + L);
            float NdotH = max(H.z, 0.0f);
            float VdotH = max(dot(V, H), 0.0f);

            if (NdotH < FloatMin || VdotH < FloatMin) {
                return 0.0f;
            }

            float Alpha = Roughness * Roughness;
            float Alpha2 = Alpha * Alpha;
            
            // GGX normal distribution D(H)
            float CosTheta = H.z;
            float Cos2Theta = CosTheta * CosTheta;
            float Tan2Theta = (1.0f - Cos2Theta) / Cos2Theta;
            float D = Alpha2 / (PI * Cos2Theta * Cos2Theta * (Alpha2 + Tan2Theta) * (Alpha2 + Tan2Theta));

            // Specular PDF (GGX) - same as BRDFSamplingStrategy::PDF
            float PDFSpec = D * NdotH / (4.0f * VdotH);

            return PDFSpec;
        }

    };
} // namespace Omni
