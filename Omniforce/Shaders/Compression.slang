module Compression;

import Common;
import Gen.RenderingGenerated;
import Rendering;

namespace Omni {

public struct VertexDecoder {
    public static float3 Decode(uint Bitrate, float3 ClusterCenter, int3 EncodedVertex) {
        int BitExtendMask = 1 << (Bitrate - 1);
        EncodedVertex = (EncodedVertex ^ BitExtendMask) - BitExtendMask;

        return ldexp(float3(EncodedVertex), int3(-Bitrate)) + ClusterCenter;
	}
}

}