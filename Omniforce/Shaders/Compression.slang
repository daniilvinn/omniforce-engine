module Compression;

import Gen.Common;
import Gen.RenderingGenerated;
import Core.Rendering;

namespace Omni {

public struct VertexDecoder {
    public float3 Decode(ClusterGeometryMetadata InMetadata, MeshClusterBounds CullData, int3 InEncodedVertex) {
        int BitExtendMask = 1 << (InMetadata.Bitrate - 1);
        InEncodedVertex = (InEncodedVertex ^ BitExtendMask) - BitExtendMask;

        return ldexp(float3(InEncodedVertex), int3(-InMetadata.Bitrate)) + CullData.vis_culling_sphere.center;
	}
}

}
