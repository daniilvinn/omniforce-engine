#pragma once

import Common;

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

    float2 Ndc = (float2(PixelIndex) / float2(ImageSize)) * 2.0 - 1.0;

    float3 Origin = float3(0.0, 0.0, -5.0);
    float3 Direction = normalize(float3(Ndc.x, -Ndc.y, 1.0));

    RayDesc Ray;
    Ray.Origin = Origin;
    Ray.Direction = Direction;
    Ray.TMin = 0.001;
    Ray.TMax = 10000.0;

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

    SceneVisibilityBuffer[PixelIndex] = uint64_t(Payload.Color.x) * uint64_t(-1ull);
}

[shader("closesthit")]
void PathVertex(inout RayPayload Payload, in BuiltInTriangleIntersectionAttributes Attr)
{
    Payload.Color = float3(0.0, 0.0, 0.0);
}

[shader("miss")]
void Miss(inout RayPayload Payload)
{
    Payload.Color = float3(1.0, 0.0, 0.0);
}