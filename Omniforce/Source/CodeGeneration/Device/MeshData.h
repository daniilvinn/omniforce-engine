#pragma once

#include <Foundation/Common.h>

#include <Renderer/Meshlet.h>
#include <CodeGeneration/Device/BDA.h>

namespace Omni {

    struct META(ShaderExpose, Module = "RenderingGenerated") MeshData {
        float32 lod_distance_multiplier;
        Sphere bounding_sphere;
        uint32 meshlet_count;
        int32 quantization_grid_size;
        BDA<byte> vertices;
        BDA<uint8> micro_indices;
        BDA<byte> attributes; // just raw data
        BDA<ClusterGeometryMetadata> meshlets_data;
        BDA<MeshClusterBounds> meshlets_cull_bounds;
    };

}