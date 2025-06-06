#pragma once

implementing Gen.RenderingGenerated;

import Gen.BasicTypes;

namespace Omni {

public struct GeometryMeshData {
	public float lod_distance_multiplier;
	public Sphere bounding_sphere;
	public uint meshlet_count;
	public int quantization_grid_size;
	public uint* vertices;
	public uint8_t* micro_indices;
	public uint8_t* attributes;
	public ClusterGeometryMetadata* meshlets_data;
	public MeshClusterBounds* meshlets_cull_bounds;
	public RayTracingGeometryData ray_tracing;
};

}
