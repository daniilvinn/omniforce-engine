#pragma once

implementing Gen.BasicTypes;


namespace Omni {

public struct Sprite {
	public float3 position;
	public half4 rotation;
	public float2 size;
	public float4 color_tint;
	public uint texture_id;
};

}
