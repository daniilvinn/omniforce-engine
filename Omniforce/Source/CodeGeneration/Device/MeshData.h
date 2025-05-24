#pragma once

#include <Foundation/Common.h>

#include <Renderer/Meshlet.h>
#include <CodeGeneration/Device/SpecialTypes.h>

namespace Omni {
    
	struct META(ShaderExpose, Module = "RenderingGenerated") GeometryAttributeMetadata {
		uint8 HasUV : 1;
		uint8 HasNormals : 1;
		uint8 HasTangents : 1;
		uint8 HasColor : 1;
		uint8 UVCount : 4;
	};

	struct META(ShaderExpose, Module = "RenderingGenerated") GeometryAttributeOffsetTable {

		uint8 Normal;
		uint8 Tangent;
		uint8 Color;
		uint8 UV[15];
	};

	struct META(ShaderExpose, Module = "RenderingGenerated") GeometryLayoutTable {
		GeometryAttributeMetadata AttributeMask;
		uint32 Stride;
		GeometryAttributeOffsetTable Offsets;
	};

    struct META(ShaderExpose, Module = "RenderingGenerated") RayTracingGeometryData {
        BDA<glm::uvec3> indices;
		GeometryLayoutTable layout;
        BDA<byte> attributes;
    };

    struct META(ShaderExpose, Module = "RenderingGenerated") GeometryMeshData {
        float32 lod_distance_multiplier;
        Sphere bounding_sphere;
        uint32 meshlet_count;
        int32 quantization_grid_size;
        BDA<uint32> vertices;
        BDA<uint8> micro_indices;
        BDA<byte> attributes; // just raw data
        BDA<ClusterGeometryMetadata> meshlets_data;
        BDA<MeshClusterBounds> meshlets_cull_bounds;
        RayTracingGeometryData ray_tracing;
    };

}