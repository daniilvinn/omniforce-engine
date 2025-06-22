#pragma once

module NEE;

import Common;
import Lighting;
import Gen.RenderingGenerated;
import Sampler;

namespace Omni {
    public struct NextEventEstimator {
        [mutating]
        public void Setup(ScenePointLights *InLightList) {
            PointLightList = InLightList;
        }

        public LightSample Sample(float2 Xi, float3 P, float3 N) {
            LightSample Sample;
            Sample.Valid = false;

            PointLight Light = SelectLight(Xi.x);

            // Sample a point on hemisphere with uniform probability
            UniformHemisphereSampler Sampler;
            float3 L = Sampler.Sample(Xi);

            // Transform to visible hemisphere from P
            TangentSpace TangentSpace = TangentSpace(normalize(P - Light.Position));
            L = mul(L, TangentSpace.TBN);

            // Setup sample
            Sample.Position = L * Light.Radius + Light.Position;
            Sample.Normal = L;
            Sample.PDF = 1 / (2 * PI * pow(Light.Radius, 2)); // Compute PDF over half of the light's area since we sample hemisphere
            Sample.Valid = dot(N, normalize(Sample.Position - P)) > 0; // Check if sample is inside N hemisphere
            Sample.LightPosition = Light.Position;

            return Sample;
        }

        // Returns index of selected light; Uniform probability for all light sources
        public uint SelectLightIndex(float Random) {
            return min(uint(float(PointLightList.Count) * Random), PointLightList.Count - 1);
        }

        public PointLight SelectLight(float Random) {
            return PointLightList.Lights[min(uint(float(PointLightList.Count) * Random), PointLightList.Count - 1)];
        }
        
        private ScenePointLights* PointLightList;
    }
}