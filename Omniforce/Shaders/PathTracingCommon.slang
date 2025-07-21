module PathTracingCommon;

import Common;
import Lighting;
import Rendering;
import Compression;
import Random;
import Gen.PathTracingInput;

using namespace Omni;

namespace Omni {

public struct RayPayload
{
    public float3 Radiance;
    public float3 PathThroughput;
    public float3 IncomingDirection;
    public float3 HitPoint;
    public uint RandomSeed;
    public float BSDFPdf;
    public float CurrentMediumIOR;
};

public struct HitSurfaceInfo<SurfaceDomain Domain = SurfaceDomain.Opaque> {
    public bool Valid;
    public PBRSurface<Domain> Surface;

    [mutating]
    public bool Gather(RayPayload Payload, BuiltInTriangleIntersectionAttributes Attr) {
        InstanceRenderData Instance = Input.Instances[InstanceID()];
        GeometryMeshData Geometry = Input.Meshes[Instance.geometry_data_id];
        GeometryLayoutTable Layout = Geometry.ray_tracing.layout;
        uint3 HitTriangleIndices = Geometry.ray_tracing.indices[PrimitiveIndex()];

        float3 Barys = float3(1.0 - Attr.barycentrics.x - Attr.barycentrics.y, Attr.barycentrics);

        RTMaterial<Domain> *Material = (RTMaterial<Domain>*)Instance.material_address;

        // Fetch triangle positions
        float3 WorldPosition = 0;
        float4 TriangleVerticesClip[3];

        for (uint i = 0; i < 3; i++) {
            float3 Vertex = HitTriangleVertexPosition(i);

            WorldPosition += Vertex * Barys[i];
            TriangleVerticesClip[i] = mul(Input.View.view_proj, float4(Vertex, 1.0f));
        }
        WorldPosition = Instance.transform.TransformPoint(WorldPosition);

        uint LastFetchedUVIndex = (uint)-1;

        // Calculate analytic derivatives
        float2 ImageSize;
        OutputImage.GetDimensions(ImageSize.x, ImageSize.y);

        uint3 PixelIndex = DispatchRaysIndex();

        float2 PixelNDC = (float2(PixelIndex.xy) + 0.5) / float2(Input.View.viewport_width, Input.View.viewport_height) * 2.0 - 1.0;
        PixelNDC.y = -PixelNDC.y;

        BarycentricDerivative Gradient;
        Gradient.Setup(TriangleVerticesClip[0], TriangleVerticesClip[1], TriangleVerticesClip[2], PixelNDC, ImageSize);

        // Fetch base color
        float2 UV = 0;
        float2 Ddx = 0;
        float2 Ddy = 0;

        float4 BaseColor = Material.UniformColor;
        if (Material.Metadata.HasBaseColor == 1) {
            UV = Geometry.RTFetchUV(HitTriangleIndices, Material.BaseColor.UVChannelIndex, Barys);
            Geometry.RTFetchUVGradients(HitTriangleIndices, Material.BaseColor.UVChannelIndex, Gradient, Ddx, Ddy);

            BaseColor = TextureHeap[Material.BaseColor.TextureIndex].SampleGrad(UV, Ddx, Ddy);
            LastFetchedUVIndex = Material.BaseColor.UVChannelIndex;
        }

        // Fetch vertex colors
        if (Layout.AttributeMask.HasColor == 1) {
            VertexDecoder Decoder;

            float4 VertexColor = 0;
            for (uint i = 0; i < 3; i++) {
                uint AttrOffset = Layout.Stride * HitTriangleIndices[i];
                uint8_t4 FetchedColor = *(uint8_t4 *)(Geometry.ray_tracing.attributes + AttrOffset + Layout.Offsets.Color);
                VertexColor += Decoder.DecodeColor(FetchedColor) * Barys[i];
            }
            BaseColor *= VertexColor;
        }

        // Fetch vertex normals
        float3 GeometryNormal = 0;
        float3 TrueGeometryNormal = 0;
        {
            float3 v0 = HitTriangleVertexPosition(0);
            float3 v1 = HitTriangleVertexPosition(1);
            float3 v2 = HitTriangleVertexPosition(2);

            float3 edge1 = v1 - v0;
            float3 edge2 = v2 - v0;

            TrueGeometryNormal = normalize(cross(edge1, edge2));
        }

        if (Layout.AttributeMask.HasNormals == 1) {
            VertexDecoder Decoder;

            for (uint i = 0; i < 3; i++) {
                uint AttrOffset = Layout.Stride * HitTriangleIndices[i];
                half2 FetchedNormal = *(half2 *)(Geometry.ray_tracing.attributes + AttrOffset + Layout.Offsets.Normal);
                GeometryNormal += Decoder.OctahedronDecode(FetchedNormal) * Barys[i];
            }
  
            if (
                dot(QuaternionRotate(Instance.transform.rotation, GeometryNormal), -Payload.IncomingDirection) < 0
                && Domain == SurfaceDomain.Opaque
            ) {
                if (Material.Metadata.DoubleSided == 1) {
                    GeometryNormal = -GeometryNormal;
                } else {
                    NormalFixer Fixer;
                    GeometryNormal = Fixer.PullNormal(GeometryNormal, Payload.IncomingDirection, TrueGeometryNormal);
                }
            }
        } else {
            GeometryNormal = TrueGeometryNormal;
        }

        // Fetch normal map
        float4 T = 0.0;
        float3 B = 0.0;
        float3 N = normalize(GeometryNormal);
        if (Material.Metadata.HasNormal == 1 && Layout.AttributeMask.HasTangents == 1) {
            // Check if corresponding UVs are loaded; reload otherwise
            if (LastFetchedUVIndex != Material.Normal.UVChannelIndex) {
                UV = Geometry.RTFetchUV(HitTriangleIndices, Material.Normal.UVChannelIndex, Barys);
                Geometry.RTFetchUVGradients(HitTriangleIndices, Material.BaseColor.UVChannelIndex, Gradient, Ddx, Ddy);
                LastFetchedUVIndex = Material.BaseColor.UVChannelIndex;
            }

            // Fetch tangents
            VertexDecoder Decoder;
            for (uint i = 0; i < 3; i++) {
                uint AttrOffset = Layout.Stride * HitTriangleIndices[i];
                half2 FetchedTangent = *(half2 *)(Geometry.ray_tracing.attributes + AttrOffset + Layout.Offsets.Tangent);
                T += Decoder.DecodeTangent(FetchedTangent) * Barys[i];
            }
            T = normalize(T);

            // Compute bitangent
            B = cross(GeometryNormal, T.xyz) * T.w;

            // Fetch normal map, decode and transform
            float3 NormalMapTexel = TextureHeap[Material.Normal.TextureIndex].SampleGrad(UV, Ddx, Ddy).rgb;
            NormalMapTexel = Decoder.DecodeNormalMap(NormalMapTexel);

            // Build TBN matrix for a normal from normal map
            float3x3 TBN = float3x3(
                T.xyz,
                B,
                GeometryNormal
            );

            N = mul(NormalMapTexel, TBN);

            NormalFixer Fixer;
            N = Fixer.PullNormal(N, Payload.IncomingDirection, GeometryNormal);

            T.xyz = normalize(T.xyz - dot(T.xyz, N) * N);
            B = cross(N, T.xyz);
        } else {
            TangentSpace TangentSpace = TangentSpace(N);
            T.xyz = TangentSpace.T;
            B = TangentSpace.B;
        }

        // Setup tangent space
        TangentSpace TangentSpace = TangentSpace(T.xyz, B, N, Instance.transform.rotation);

        // Fetch metallic roughness
        float3 MetallicRoughnessOcclusion = float3(1.0f, 1.0f, 0.0f);
        if (Material.Metadata.HasMetallicRoughness == 1) {
            if (LastFetchedUVIndex != Material.MetallicRoughness.UVChannelIndex) {
                UV = Geometry.RTFetchUV(HitTriangleIndices, Material.MetallicRoughness.UVChannelIndex, Barys);
                Geometry.RTFetchUVGradients(HitTriangleIndices, Material.BaseColor.UVChannelIndex, Gradient, Ddx, Ddy);
                LastFetchedUVIndex = Material.BaseColor.UVChannelIndex;
            }

            MetallicRoughnessOcclusion = TextureHeap[Material.MetallicRoughness.TextureIndex].SampleGrad(UV, Ddx, Ddy).rgb;
        }

        Surface.TangentSpace = TangentSpace;
        Surface.Position = WorldPosition;
        Surface.Color = BaseColor.rgb;
        Surface.Metallic = MetallicRoughnessOcclusion.b;
        Surface.Roughness = MetallicRoughnessOcclusion.g;

        if (let Transmission = Material.TransmissionData.get()) {
            Surface.Transmission.Factor = 1.0 - BaseColor.a; // TODO: check if this is correct
            Surface.Transmission.IOR = Transmission.IOR;
            Surface.Transmission.Thickness = Transmission.Thickness;
        }
        
        return true;
    }
}

}