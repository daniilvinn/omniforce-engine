#pragma once

import Common;
import Gen.PathTracingInput;
import Lighting;
import Compression;
import Rendering;

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

    RTMaterial* Material = (RTMaterial*)Instance.material_address;

    // Early out if no UVs assigned
    if (Layout.AttributeMask.HasUV == 0) {
        return;
    }

    // Fetch triangle positions
    float3 WorldPosition = 0;
    for (uint i = 0; i < 3; i++) {
        WorldPosition += HitTriangleVertexPosition(i) * Barys[i];
    }
    WorldPosition = Instance.transform.TransformPoint(WorldPosition);

    uint LastFetchedUVIndex = (uint)-1;

    // Fetch base color
    float2 UV = 0;
    float3 BaseColor = 0.2;
    if (Material.Metadata.HasBaseColor == 1) {
        UV = Geometry.RTFetchUV(HitTriangleIndices, Material.BaseColor.UVChannelIndex, Barys);
        BaseColor = TextureHeap[Material.BaseColor.TextureIndex].SampleLevel(UV, 0.0).rgb;
        LastFetchedUVIndex = Material.BaseColor.UVChannelIndex;
    }

    // Fetch vertex normals
    half3 Normal = 0;
    if (Material.Metadata.HasNormal == 1) {
        VertexDecoder Decoder;

        for (uint i = 0; i < 3; i++) {
            uint AttrOffset = Layout.Stride * HitTriangleIndices[i];
            half2 FetchedNormal = *(half2 *)(Geometry.ray_tracing.attributes + AttrOffset + Layout.Offsets.Normal);
            Normal += Decoder.OctahedronDecode(FetchedNormal) * Barys[i];
        }
    } else {
        Normal = half3(0, 0, 1);
    }

    // Fetch normal map
    if (Material.Metadata.HasNormal == 1 && Layout.AttributeMask.HasTangents == 1) {
        // Check if corresponding UVs are loaded; reload otherwise
        if (LastFetchedUVIndex != Material.Normal.UVChannelIndex) {
            UV = Geometry.RTFetchUV(HitTriangleIndices, Material.Normal.UVChannelIndex, Barys);
            LastFetchedUVIndex = Material.BaseColor.UVChannelIndex;
        }

        // Fetch tangents
        VertexDecoder Decoder;
        half4 Tangent = 0;
        for (uint i = 0; i < 3; i++) {
            uint AttrOffset = Layout.Stride * HitTriangleIndices[i];
            half2 FetchedTangent = *(half2 *)(Geometry.ray_tracing.attributes + AttrOffset + Layout.Offsets.Tangent);
            Tangent.xyz += Decoder.OctahedronDecode(FetchedTangent) * Barys[i];
        }

        // Compute bitangent
        half3 Bitangent = cross(Normal, Tangent.xyz) * Tangent.w;

        // Fetch normal map, decode and transform
        half3 NormalMapTexel = TextureHeap[Material.Normal.TextureIndex].SampleLevel(UV, 0.0).rgb;
        NormalMapTexel = Decoder.DecodeNormalMap(NormalMapTexel);

        half3x3 TBN = half3x3(
            Tangent.xyz,
            Bitangent,
            Normal
        );

        Normal = mul(TBN, NormalMapTexel);
    }
    float3 N = QuaternionRotate(float4(Instance.transform.rotation), Normal);
    Payload.Color = (N + 1) * 0.5;
    return;

    // Fetch metallic roughness
    float3 MetallicRoughnessOcclusion = float3(1.0f, 0.5f, 0.0f);
    if (Material.Metadata.HasMetallicRoughness == 1) {
        if (LastFetchedUVIndex != Material.MetallicRoughness.UVChannelIndex) {
            UV = Geometry.RTFetchUV(HitTriangleIndices, Material.MetallicRoughness.UVChannelIndex, Barys);
            LastFetchedUVIndex = Material.BaseColor.UVChannelIndex;
        }

        MetallicRoughnessOcclusion = TextureHeap[Material.MetallicRoughness.TextureIndex].SampleLevel(UV, 0.0).rgb;
    }

    float3 V = normalize(Input.View.position - WorldPosition);

    float3 F0 = 0.04f;
    F0 = lerp(F0, BaseColor, MetallicRoughnessOcclusion.b);

    float3 Lo = 0.0f;
    for (int i = 0; i < Input.PointLights.Count; i++) {
        PointLight Light = Input.PointLights.Lights[i];

        float3 L = normalize(Light.Position - WorldPosition);
        float3 H = normalize(V + L);
        float Distance = length(Light.Position - WorldPosition);

        float Attenuation = 1.0;//Light.ComputeAttenuation(WorldPosition);
        float3 Radiance = Light.Color * Attenuation * Light.Intensity;

        float NDF = DistributionGGX(N, H, MetallicRoughnessOcclusion.g);
        float G = GeometrySmith(N, V, L, MetallicRoughnessOcclusion.g);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

        float3 Numerator = NDF * G * F;
        float Denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        
        float3 Specular = Numerator / Denominator;

        float3 kS = F;
        float3 kD = Invert(kS);
        kD *= 1.0f - MetallicRoughnessOcclusion.b;

        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * BaseColor / PI + Specular) * Radiance * NdotL;
    }

    float3 Ambient = 0.1 * BaseColor;
    float3 Color = Ambient * Lo;
    Color = Color * MetallicRoughnessOcclusion.r;
    Color = Color / (Color + 1.0f);
    Color = pow(Color, float3(1.0 / 2.2));

    Payload.Color = Color;
}

[shader("closesthit")]
void PointLightHit(inout RayPayload Payload, in BuiltInTriangleIntersectionAttributes Attr) {
    PointLight Light = Input.PointLights.Lights[InstanceID()];
    Payload.Color = Light.Color;
}

[shader("miss")]
void Miss(inout RayPayload Payload)
{
    Payload.Color = 0.1;
    return;

    if (WorldRayDirection().y >= 0.0f) {
        Payload.Color = float3(0.7, 0.9, 1.0);
    } else {
        Payload.Color = float3(0.0, 0.0, 0.0);
    }
        
}