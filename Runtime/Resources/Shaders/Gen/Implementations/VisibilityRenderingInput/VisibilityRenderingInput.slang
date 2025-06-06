#pragma once

implementing Gen.VisibilityRenderingInput;

import Gen.SoftwareRasterizerGenerated;
import Gen.BasicTypes;
import Gen.RenderingGenerated;

namespace Omni {

public struct VisibilityRenderingInput {
	public ViewData* View;
	public GeometryMeshData* Meshes;
	public InstanceRenderData* Instances;
	public SceneVisibleClusters* OutputVisibleClusters;
	public SWRasterQueue* SoftwareRasterQueue;
};

[[vk::push_constant]]
public ConstantBuffer<VisibilityRenderingInput> Input;

}
