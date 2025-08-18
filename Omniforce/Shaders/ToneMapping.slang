#pragma once

module ToneMapping;

import Common;
import Gen.ToneMappingInput;
import FullScreenBase;

using namespace Omni;

struct PSOutput {
	float4 Color : SV_TARGET0;
}

float3 ApplyToneMapping(float3 Color, float Exposure) {
    float3 Mapped = Color * Exposure;
    return Mapped / (Mapped + 1.0f);
}

float3 ApplyGammaCorrection(float3 Color, float Gamma)
{
    float3 lower = Color * 12.92;
    float3 upper = 1.055 * pow(Color, rcp(2.4)) - 0.055;
    return select(Color <= 0.0031308, lower, upper);
}

[shader("fragment")]
PSOutput FragmentMain(VSOutput StageInput) {
    uint2 ImageSize;
    OutputImage.GetDimensions(ImageSize.x, ImageSize.y);

    uint2 PixelCoord = StageInput.ScreenUV * ImageSize;

    float3 Color = OutputImage[PixelCoord].rgb;
    Color = ApplyToneMapping(Color, Input.View.exposure);
    Color = ApplyGammaCorrection(Color, Input.View.gamma);

    PSOutput Out;
    Out.Color = float4(Color, 1.0f);

    return Out;
}