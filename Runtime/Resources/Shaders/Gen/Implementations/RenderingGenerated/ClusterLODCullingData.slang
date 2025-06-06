#pragma once

implementing Gen.RenderingGenerated;

import Gen.BasicTypes;

namespace Omni {

public struct ClusterLODCullingData {
	public Sphere sphere;
	public Sphere parent_sphere;
	public float error;
	public float parent_error;
};

}
