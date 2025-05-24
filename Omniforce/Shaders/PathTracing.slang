#pragma once

import Common;
import Gen.PathTracingInput;

struct RayPayload
{
    float3 Color;
};

using namespace Omni;

[shader("raygeneration")]
void PathConstruction()
{
    uint2 PixelIndex = DispatchRaysIndex().xy;
    uint2 ImageSize = DispatchRaysDimensions().xy;

    ViewData* Camera = Input.View;

    float2 Ndc = (float2(PixelIndex) + 0.5) / float2(Camera.viewport_width, Camera.viewport_height) * 2.0 - 1.0;
    Ndc.y = -Ndc.y;

    float AspectRatio = Camera.viewport_width / Camera.viewport_height;
    float ScaleY = tan(Camera.fov * 0.5);
    float ScaleX = ScaleY * AspectRatio;

    float3 RayDirViewSpace = normalize(float3(Ndc.x * ScaleX, Ndc.y * ScaleY, -1.0));
    float3 Direction = normalize(mul(transpose(Camera.view), float4(RayDirViewSpace, 0.0)).xyz);
    float3 Origin = Camera.position;

    RayDesc Ray;
    Ray.Origin = Origin;
    Ray.Direction = Direction;
    Ray.TMin = 0.001;
    Ray.TMax = Camera.far_clip_distance;

    RayPayload Payload;
    Payload.Color = float3(0.0, 0.0, 0.0);

    TraceRay(
        SceneTLAS,
        RAY_FLAG_NONE,
        0xFF,
        0,   
        1,   
        0,   
        Ray,
        Payload
    );

    OutputImage[PixelIndex] = float4(Payload.Color, 1.0f);
}

[shader("closesthit")]
void PathVertex(inout RayPayload Payload, in BuiltInTriangleIntersectionAttributes Attr)
{
    InstanceRenderData Instance = Input.Instances[InstanceID()];
    GeometryMeshData Geometry = Input.Meshes[Instance.geometry_data_id];
    GeometryLayoutTable Layout = Geometry.ray_tracing.layout;
    uint3 HitTriangleIndices = Geometry.ray_tracing.indices[PrimitiveIndex()];

    float3 Barys = float3(1.0 - Attr.barycentrics.x - Attr.barycentrics.y, Attr.barycentrics);

    // Handle UV coordinates
    float2 UV = 0;
    if (Layout.AttributeMask.HasUV == true) {
        for (uint i = 0; i < 3; i++) {
            uint AttrOffset = Layout.Stride * HitTriangleIndices[i];
            half2 FetchedUV = *(half2*)(Geometry.ray_tracing.attributes + AttrOffset + Layout.Offsets.UV[0]);
            UV += FetchedUV * Barys[i];
        }
    }
    UV.y = 1.0f - UV.y;

    RTMaterial* Material = (RTMaterial*)Instance.material_address;

    if (Material.Metadata.HasBaseColor == true) {
        float3 Texel = TextureHeap[NonUniformResourceIndex(Material.BaseColor.TextureIndex)].SampleLevel(UV, 0.0).rgb; // hard-coded texture index
        Payload.Color = float3(Texel);
    }
}

[shader("miss")]
void Miss(inout RayPayload Payload)
{
    if (WorldRayDirection().y >= 0.0f) {
        Payload.Color = float3(0.7, 0.9, 1.0);
    } else {
        Payload.Color = float3(0.0, 0.0, 0.0);
    }
        
}