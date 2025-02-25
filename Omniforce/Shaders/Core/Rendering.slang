module Rendering;

import Gen.Common;
import Gen.RenderingGenerated;

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

	public extension Omni.MeshData {

		int ReadVertexChannel(uint InClusterID, uint InVertexID, uint VertexChannel)
        {
            int Result = 0;

			ClusterGeometryMetadata ClusterMetadata = meshlets_data[InClusterID];
			uint LocalOffset = ClusterMetadata.Bitrate * InVertexID + ClusterMetadata.Bitrate * InVertexID;
			uint GlobalOffset = ClusterMetadata.vertex_bit_offset + LocalOffset;

            uint BitstreamNodeIndex = GlobalOffset >> 5;
            uint BitstreamNodeOffset = GlobalOffset & 0x1F;

            uint Bitmask = ((1u << ClusterMetadata.Bitrate) - 1) << BitstreamNodeOffset;

            Result = (vertices[BitstreamNodeIndex] & Bitmask) >> BitstreamNodeOffset;

            if (BitstreamNodeOffset + ClusterMetadata.Bitrate > 32u) {
                Bitmask = ((1u << (ClusterMetadata.Bitrate - (32u - BitstreamNodeOffset))) - 1);
                Result |= (vertices[BitstreamNodeIndex + 1] & Bitmask) << (32u - BitstreamNodeOffset);
			}

            return Result;
        }

	}

}