#pragma once

module NextEventEstimation;

import Common;
import Lighting;
import Gen.RenderingGenerated;
import Sampler;

namespace Omni {
    public struct NextEventEstimator {

        public __init(ScenePointLights *InLightList) {
            PointLightList = InLightList;
        }

        public LightSample Sample(float2 Xi, float3 P, float3 N) {
            LightSample Sample;
            Sample.Valid = false;

            if (PointLightList.Count == 0) {
                return Sample;
            }

            PointLight Light = SelectLight(Xi.x);

            // Create tangent space oriented towards the surface point P
            float3 LightToSurface = normalize(P - Light.Position);
            TangentSpace LightTangentSpace = TangentSpace(LightToSurface);

            // Sample hemisphere oriented towards surface P
            UniformHemisphereSampler HemisphereSampler;
            float3 LocalDirection = HemisphereSampler.Sample(Xi);

            // Transform to world space
            float3 WorldDirection = normalize(mul(LocalDirection, LightTangentSpace.TBN));

            // Setup sample
            Sample.Position = Light.Position + WorldDirection * Light.Radius;
            Sample.Normal = WorldDirection;
            Sample.PDF = 1 / (2 * PI * pow(Light.Radius, 2)); // PDF over hemisphere area
            Sample.Valid = dot(N, normalize(Sample.Position - P)) > 0; // Check if sample is inside surface N hemisphere
            Sample.LightPosition = Light.Position;
            Sample.Color = Light.Color;
            Sample.Intensity = Light.Intensity;

            return Sample;
        }

        // Returns index of selected light; Uniform probability for all light sources
        public uint SelectLightIndex(float Random) {
            return min(uint(float(PointLightList.Count) * Random), PointLightList.Count - 1);
        }

        public PointLight SelectLight(float Random) {
            return PointLightList.Lights[min(uint(float(PointLightList.Count) * Random), PointLightList.Count - 1)];
        }

        public uint GetLightCount() {
            return PointLightList.Count;
        }

        // Calculate NEE PDF for a given direction from surface P towards a specific light
        public float CalculateNEEPDF(float3 P, float3 Direction, uint LightIndex) {
            if (LightIndex >= PointLightList.Count) {
                return 0.0f;
            }
            
            PointLight Light = PointLightList.Lights[LightIndex];
            
            // Calculate intersection point with light sphere
            float3 LightToP = P - Light.Position;
            float A = dot(Direction, Direction);
            float B = 2.0f * dot(LightToP, Direction);
            float C = dot(LightToP, LightToP) - Light.Radius * Light.Radius;
            
            float Discriminant = B * B - 4.0f * A * C;
            if (Discriminant < 0.0f) {
                return 0.0f; // No intersection
            }
            
            float T = (-B - sqrt(Discriminant)) / (2.0f * A);
            if (T <= 0.0f) {
                return 0.0f; // Behind surface
            }
            
            float3 HitPoint = P + Direction * T;
            float3 LightToHitPoint = normalize(HitPoint - Light.Position);
            float3 LightToSurface = normalize(P - Light.Position);
            
            // Check if hit point is on the hemisphere visible from P
            if (dot(LightToHitPoint, LightToSurface) < 0.0f) {
                return 0.0f; // On back hemisphere
            }
            
            // Light selection probability (uniform)
            float LightSelectionPDF = 1.0f / float(PointLightList.Count);
            
            // Area PDF on hemisphere (uniform hemisphere sampling)
            float HemisphereAreaPDF = 1.0f / (2.0f * PI * Light.Radius * Light.Radius);
            
            // Convert to solid angle PDF
            float Distance = length(HitPoint - P);
            float DistanceSquared = Distance * Distance;
            float CosTheta = max(dot(LightToHitPoint, -Direction), 0.0f);
            
            if (CosTheta <= 0.0f) {
                return 0.0f;
            }
            
            float SolidAnglePDF = HemisphereAreaPDF * DistanceSquared / CosTheta;
            
            return LightSelectionPDF * SolidAnglePDF;
        }

        // Calculate NEE PDF for a given direction from surface P towards a specific light
        public float CalculateNEEPDF(float3 P, float3 Direction, uint LightIndex, float3 HitPoint) {
            PointLight Light = PointLightList.Lights[LightIndex];

            float3 LightToHitPoint = normalize(HitPoint - Light.Position);
            float3 LightToSurface = normalize(P - Light.Position);

            // Check if hit point is on the hemisphere visible from P
            if (dot(LightToHitPoint, LightToSurface) < 0.0f) {
                return 0.0f; // On back hemisphere
            }

            // Light selection probability (uniform)
            float LightSelectionPDF = 1.0f / float(PointLightList.Count);

            // Area PDF on hemisphere (uniform hemisphere sampling)
            float HemisphereAreaPDF = 1.0f / (2.0f * PI * Light.Radius * Light.Radius);

            // Convert to solid angle PDF
            float Distance = length(HitPoint - P);
            float DistanceSquared = Distance * Distance;
            float CosTheta = max(dot(LightToHitPoint, -Direction), 0.0f);

            if (CosTheta <= 0.0f) {
                return 0.0f;
            }

            float SolidAnglePDF = HemisphereAreaPDF * DistanceSquared / CosTheta;

            return LightSelectionPDF * SolidAnglePDF;
        }

        private ScenePointLights* PointLightList;
    }
}