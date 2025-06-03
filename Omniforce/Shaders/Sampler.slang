#pragma once

module Sampler;

import Common;

namespace Omni {

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

    public struct CosineWeightedHemisphereSampler {
        public float3 Sample(float2 u) {
            float2 d = ConcentricDiskSampler().Sample(u);
            float z = sqrt(max(0.0, 1.0 - d.x * d.x - d.y * d.y));
            return float3(d.x, d.y, z);
        }
    }

    public struct GGXVNDFSampler {
        public float3 Sample(float3 ViewDir, float Alpha, float2 Xi)
        {
            // 1. Transform view vector to hemisphere configuration
            float3 V = normalize(ViewDir);
            float3 Vh = normalize(float3(Alpha * V.x, Alpha * V.y, V.z));

            // 2. Orthonormal basis
            float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
            float3 T1 = lensq > 0.0 ? normalize(float3(-Vh.y, Vh.x, 0.0)) : float3(1.0, 0.0, 0.0);
            float3 T2 = cross(Vh, T1);

            // 3. Sample point on disk
            float r = sqrt(Xi.x);
            float phi = 2.0 * PI * Xi.y;
            float t1 = r * cos(phi);
            float t2 = r * sin(phi);

            // 4. Reproject onto hemisphere
            float s = 0.5f * (1.0f + Vh.z);
            float t3 = sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2));
            float3 Nh = t1 * T1 + t2 * T2 + t3 * Vh;

            // 5. Transform normal back to ellipsoid space
            float3 H = normalize(float3(Alpha * Nh.x, Alpha * Nh.y, max(0.0, Nh.z)));
            return H;
        }
    }
}