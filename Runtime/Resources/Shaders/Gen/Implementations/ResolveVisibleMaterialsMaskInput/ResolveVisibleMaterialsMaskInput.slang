#pragma once

implementing Gen.ResolveVisibleMaterialsMaskInput;

import Gen.RenderingGenerated;

namespace Omni {

public struct ResolveVisibleMaterialsMaskInput {
	public InstanceRenderData* Instances;
	public SceneVisibleCluster* VisibleClusters;
};

[[vk::push_constant]]
public ConstantBuffer<ResolveVisibleMaterialsMaskInput> Input;

}
