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

}