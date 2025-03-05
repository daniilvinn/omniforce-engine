module SpriteRendering;

import Gen.BasicTypes;
import Gen.SpriteRenderingInput;
import Common;

using namespace Omni;

static const float3 vertices[6] = {
	float3(-1.0f,	-1.0f,	  0.0f),
	float3( 1.0f,	-1.0f,	  0.0f),
	float3( 1.0f,	 1.0f,	  0.0f),
    float3( 1.0f,	 1.0f,	  0.0f),
	float3(-1.0f,	 1.0f,	  0.0f),
	float3(-1.0f,	-1.0f,	  0.0f)
};

static const float2 tex_coords[6] = {
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f),
	float2(1.0f, 1.0f),
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f)
};

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 TextureCoordinate;
    uint TextureIndex;
    float4 ColorTint;
};

[shader("vertex")]
VSOutput VertexStage(uint InstanceID: SV_InstanceID, uint VertexID: SV_VertexID) {
    Sprite CurrentSprite = Input.Sprites[InstanceID];

    Transform SpriteTransform;
    SpriteTransform.translation = CurrentSprite.position;
    SpriteTransform.rotation = CurrentSprite.rotation;
    SpriteTransform.scale = float3(CurrentSprite.size, 1.0);

    float3 TransformedVertex = SpriteTransform.TransformPoint(vertices[VertexID]);
    
    VSOutput Output;
    Output.Position = mul(Input.View.view_proj, float4(TransformedVertex, 1.0f));
    Output.TextureCoordinate = tex_coords[VertexID];
    Output.TextureIndex = CurrentSprite.texture_id;
    Output.ColorTint = CurrentSprite.color_tint;

    return Output;
}

struct PSOutput {
    float4 Color : SV_TARGET0;
}

[shader("pixel")]
PSOutput FragmentStage(VSOutput StageInput) {
    float4 TexelColor = TextureHeap[NonUniformResourceIndex(StageInput.TextureIndex)].Sample(StageInput.TextureCoordinate);

    PSOutput Output;
    Output.Color = TexelColor * StageInput.ColorTint;

    return Output;
}