#pragma once

module PBRLighting;

using namespace Omni;

import Common;
import FullScreenBase;
import Lighting;

import Gen.PBRLightingInput;

struct PSOutput {
    float4 Color : SV_TARGET0;
}

[shader("fragment")]
PSOutput LightFunction(VSOutput StageInput) {
    float3 WorldPosition                    = TextureHeap[Input.PositionTextureID].Sample(StageInput.ScreenUV).rgb;
    float3 BaseColor                        = TextureHeap[Input.ColorTextureID].Sample(StageInput.ScreenUV).rgb;
    float3 MetallicRoughnessOcclusion       = TextureHeap[Input.MetallicRoughnessOcclusionTextureID].Sample(StageInput.ScreenUV).rgb;
    float3 N                                = TextureHeap[Input.NormalTextureID].Sample(StageInput.ScreenUV).rgb;
    float3 V                                = normalize(Input.View.position - WorldPosition);

    float3 F0 = float3(0.04f);
    F0 = lerp(F0, BaseColor, MetallicRoughnessOcclusion.b);

    float3 Lo = 0.0f;
    for (int i = 0; i < Input.PointLightCount; i++) {
        PointLight Light = Input.PointLights[i];

        float3 L = normalize(Light.Position - WorldPosition);
        float3 H = normalize(V + L);
        float Distance = length(Light.Position - WorldPosition);

        float Attenuation = Light.ComputeAttenuation(WorldPosition);
        float3 Radiance = Light.Color * Attenuation * Light.Intensity;

        float NDF = DistributionGGX(N, H, MetallicRoughnessOcclusion.g);
        float G = GeometrySmith(N, V, L, MetallicRoughnessOcclusion.g);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

        float3 Numerator = NDF * G * F;
        float Denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;

        float3 Specular = Numerator / Denominator;

        float3 kS = F;
        float3 kD = Invert(kS);
        kD *= 1.0f - MetallicRoughnessOcclusion.b;

        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * BaseColor / PI + Specular) * Radiance * NdotL;
    }

    float3 Ambient = float3(0.1) * BaseColor;
    float3 Color = Ambient * Lo;
    Color = Color * MetallicRoughnessOcclusion.r;
    Color = Color / (Color + 1.0f);
    Color = pow(Color, float3(1.0 / 2.2));

    PSOutput Output;
    Output.Color = float4(Color, 1.0f);

    return Output;
}