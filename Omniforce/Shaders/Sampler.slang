#pragma once

module Sampler;

import Common;

namespace Omni {

    public struct UniformHemisphereSampler {
        public float3 Sample(float2 U) {

            float z = U[0];
            float r = sqrt(max(1.0f - z * z, 0.0f));
            float phi = 2 * PI * U[1];
            return float3(r * cos(phi), r * sin(phi), z);
        }
    }

}