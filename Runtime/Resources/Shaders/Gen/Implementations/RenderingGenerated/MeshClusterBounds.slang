#pragma once

implementing Gen.RenderingGenerated;

import Gen.BasicTypes;

namespace Omni {

public struct MeshClusterBounds {
	public Sphere vis_culling_sphere;
	public float3 cone_apex;
	public int8_t3 cone_axis;
	public int8_t cone_cutoff;
	public ClusterLODCullingData lod_culling;
};

}
