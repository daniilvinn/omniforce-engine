#pragma once

implementing Gen.RenderingGenerated;


namespace Omni {

public struct GeometryAttributeMetadata {
	public uint8_t HasUV : 1;
	public uint8_t HasNormals : 1;
	public uint8_t HasTangents : 1;
	public uint8_t HasColor : 1;
	public uint8_t UVCount : 4;
};

}
