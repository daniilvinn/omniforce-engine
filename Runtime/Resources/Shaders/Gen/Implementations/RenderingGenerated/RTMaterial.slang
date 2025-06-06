#pragma once

implementing Gen.RenderingGenerated;


namespace Omni {

public struct RTMaterial {
	public RTMaterialMetadata Metadata;
	public DeviceTexture BaseColor;
	public DeviceTexture Normal;
	public DeviceTexture MetallicRoughness;
	public DeviceTexture Occlusion;
	public float4 UniformColor;
};

}
