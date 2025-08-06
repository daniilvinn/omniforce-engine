module Rendering;

import Common;
import Gen.BasicTypes;
import Gen.RenderingGenerated;

using namespace Omni;

namespace Omni {

	public extension Omni.ClusterGeometryMetadata {

        public static const int MESHLET_DATA_VERTEX_COUNT_BIT_OFFSET = 0;
        public static const int MESHLET_DATA_VERTEX_COUNT_BIT_COUNT = 7;

		public static const int MESHLET_DATA_TRIANGLE_COUNT_BIT_OFFSET = 7;
		public static const int MESHLET_DATA_TRIANGLE_COUNT_BIT_COUNT = 8;

		public static const int MESHLET_DATA_BITRATE_BIT_OFFSET = 15;
		public static const int MESHLET_DATA_BITRATE_BIT_COUNT = 6;

        public property uint VertexCount
		{
			get 
			{ 
				return (metadata >> MESHLET_DATA_VERTEX_COUNT_BIT_OFFSET) & ((1 << MESHLET_DATA_VERTEX_COUNT_BIT_COUNT) - 1); 
			}
		}

        public property uint TriangleCount
		{
			get 
			{
				return (metadata >> MESHLET_DATA_TRIANGLE_COUNT_BIT_OFFSET) & ((1 << MESHLET_DATA_TRIANGLE_COUNT_BIT_COUNT) - 1);
			}
		}

		public property uint Bitrate
		{
			get 
			{
				return (metadata >> MESHLET_DATA_BITRATE_BIT_OFFSET) & ((1 << MESHLET_DATA_BITRATE_BIT_COUNT) - 1);
			}
		}

	}

    public extension Omni.GeometryMeshData {

		int ReadVertexChannel(uint ClusterBitrate, uint VertexOffset)
        {
            int Result = 0;

            uint Index = VertexOffset >> 5;
            uint LocalOffset = VertexOffset & 0x1F;
            uint Bitmask = ((1 << ClusterBitrate) - 1) << LocalOffset;

            Result = (vertices[Index] & Bitmask) >> LocalOffset;

            if (LocalOffset + ClusterBitrate > 32u) {
                Bitmask = ((1 << (ClusterBitrate - (32u - LocalOffset))) - 1);
                Result |= (vertices[Index + 1] & Bitmask) << (32u - LocalOffset);
			}

            return Result;
        }

        public int3 FetchVertex(uint ClusterBitrate, uint VertexBitOffset) {
            uint3 Result;
            for (int i = 0; i < 3; i++) {
                Result[i] = ReadVertexChannel(ClusterBitrate, VertexBitOffset + ClusterBitrate * i);
            }
            return Result;
		}

        public float2 RTFetchUV(uint3 Indices, uint Index, float3 Barys) {
            half2 UV = 0;
            for (uint i = 0; i < 3; i++) {
                uint AttrOffset = ray_tracing.layout.Stride * Indices[i];
                half2 FetchedUV = *(half2 *)(ray_tracing.attributes + AttrOffset + ray_tracing.layout.Offsets.UV[Index]);
                UV += FetchedUV * Barys[i];
            }
            UV.y = 1.0hf - UV.y;

            return float2(UV);
		}

        public void RTFetchUVGradients(uint3 Indices, uint Index, BarycentricDerivative Deriv, out float2 Ddx, out float2 Ddy) {
            half2 UVs[3];
            for (uint i = 0; i < 3; i++) {
                uint AttrOffset = ray_tracing.layout.Stride * Indices[i];
                UVs[i] = *(half2 *)(ray_tracing.attributes + AttrOffset + ray_tracing.layout.Offsets.UV[Index]);
            }

            Deriv.Interpolate(UVs[0], UVs[1], UVs[2], Ddx, Ddy);
        }

	}

    public extension Omni.RTMaterial<Omni.SurfaceDomain.Transmissive> {
        public property Omni.RTMaterialTransmission Transmission {
            get {
                return TransmissionData.get().value;
            }
            private set {}
        }
    }

}