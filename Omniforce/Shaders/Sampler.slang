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
    // Uniform Sphere Sampler
    // ------------------------------------

    public struct UniformSphereSampler {
        public float3 Sample(float2 Xi)
        {
            // Map Xi.y to z = cos(theta), where theta ∈ [0, π]
            float z = 1.0f - 2.0f * Xi.y;

            // Radius of the circle at height z
            float r = sqrt(max(0.0f, 1.0f - z * z));

            // Map Xi.x to azimuthal angle phi ∈ [0, 2π]
            float phi = 2.0f * 3.14159265f * Xi.x;

            // Convert spherical coordinates to Cartesian
            float x = r * cos(phi);
            float y = r * sin(phi);

            return float3(x, y, z);
        }
    }

    // ------------------------------------
    // Uniform Hemisphere Sampler
    // ------------------------------------
    public struct UniformHemisphereSampler {
        public float3 Sample(float2 U) {
            float phi = 2.0f * PI * U.x;
            float cosTheta = U.y;
            float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
            float x = sinTheta * cos(phi);
            float y = sinTheta * sin(phi);
            float z = cosTheta;
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

    // GGX VNDF Sampler for Transmission (Walter et al. 2007)
    public struct GGXTransmissionSampler {
        public float3 Sample(float3 Wi, float3 N, float Roughness, float IOR, float2 Xi, out bool TotalInternalReflection) {
            // Step 1: Determine which side of the surface we're on
            bool Entering = dot(N, Wi) > 0.0f;
            float3 Nf = Entering ? N : -N;
            float Eta = Entering ? IOR : 1.0f / IOR;

            // Step 2: Sample microfacet normal H using existing VNDF
            GGXVNDFSampler vndf;
            float3 H = vndf.Sample(Wi, Roughness, Xi);

            // Step 3: Ensure H points to the same side as Nf
            if (dot(H, Nf) < 0.0f) {
                H = -H;
            }

            // Step 4: Refract through the microfacet (not reflect!)
            float3 Wo = refract(Wi, H, Eta);

            return Wo;
        }
    }

} // namespace Omni
