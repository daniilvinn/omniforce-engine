#pragma once

implementing Gen.RenderingGenerated;


namespace Omni {

public struct RTMaterialMetadata {
	public uint HasBaseColor : 1;
	public uint HasNormal : 1;
	public uint HasMetallicRoughness : 1;
	public uint HasOcclusion : 1;
};

}
