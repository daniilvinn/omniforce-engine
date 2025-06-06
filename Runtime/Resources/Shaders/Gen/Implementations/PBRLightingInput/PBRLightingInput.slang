#pragma once

implementing Gen.PBRLightingInput;

import Gen.BasicTypes;
import Gen.RenderingGenerated;

namespace Omni {

public struct PBRLightingInput {
	public ViewData* View;
	public PointLight* PointLights;
	public uint PointLightCount;
	public uint PositionTextureID;
	public uint ColorTextureID;
	public uint NormalTextureID;
	public uint MetallicRoughnessOcclusionTextureID;
};

[[vk::push_constant]]
public ConstantBuffer<PBRLightingInput> Input;

}
