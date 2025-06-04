#pragma once

implementing Gen.InstanceCullingInput;

import Gen.BasicTypes;
import Gen.RenderingGenerated;
import Gen.IndirectRendering;

namespace Omni {

public struct InstanceCullingInput {
	public ViewData* View;
	public GeometryMeshData* Meshes;
	public InstanceRenderData* SourceInstances;
	public uint SourceInstanceCount;
	public InstanceRenderData* PostCullInstances;
	public MeshShadingDrawParams* IndirectParams;
};

[[vk::push_constant]]
public ConstantBuffer<InstanceCullingInput> Input;

}
