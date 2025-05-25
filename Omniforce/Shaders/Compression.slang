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

    public static float3 OctahedronDecode(half2 f) {
        f = f * 2.0hf - 1.0hf;

        // https://twitter.com/Stubbesaurus/status/937994790553227264
        half3 n = half3(f.x, f.y, 1.0hf - abs(f.x) - abs(f.y));
        float16_t t = max(-n.z, 0.0hf);
        n.x += n.x >= 0.0hf ? -t : t;
        n.y += n.y >= 0.0hf ? -t : t;

        return normalize(n);
    }

    public half4 DecodeTangent(half2 f)
    {
        half4 t;
        t.xyz = OctahedronDecode(f);

        // Check if lowest bit of Y's mantissa is set. If so, bitangent sign is positive, otherwise negative.
        t.w = bool(reinterpret<uint16_t>(f.y) & 1u) ? 1.0hf : -1.0hf;

        return t;
    }

    public half3 DecodeNormalMap(half3 Normal) {
        return Normal * 2.0hf - 1.0hf;
    }

}

}