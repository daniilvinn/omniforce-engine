#pragma once

module FullScreenBase;

static const float3 vertices[6] = {
	float3(-1.0f, -1.0f, 0.0f),
	float3(1.0f, -1.0f, 0.0f),
	float3(1.0f, 1.0f, 0.0f),
	float3(1.0f, 1.0f, 0.0f),
	float3(-1.0f, 1.0f, 0.0f),
	float3(-1.0f, -1.0f, 0.0f)
};

static const float2 tex_coords[6] = {
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 0.0f),
    float2(0.0f, 1.0f)
};

public struct VSOutput {
    public float4 Position : SV_POSITION;
    public float2 ScreenUV;
};

[shader("vertex")]
public VSOutput GenerateQuad(uint VertexID: SV_VertexID) {
    VSOutput Output;

    Output.Position = float4(vertices[VertexID], 1.0f);
    Output.ScreenUV = tex_coords[VertexID];

    return Output;
}