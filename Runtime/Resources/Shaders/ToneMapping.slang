#pragma once

module ToneMapping;

import Common;
import Gen.ToneMapping;
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
	return pow(Color, 1.0 / Gamma);
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