#pragma once

implementing Gen.PathTracingInput;

import Gen.BasicTypes;
import Gen.RenderingGenerated;

namespace Omni {

public struct PathTracingInput {
	public ViewData* View;
	public InstanceRenderData* Instances;
	public GeometryMeshData* Meshes;
	public uint RandomSeed;
	public uint AccumulatedFrames;
	public ScenePointLights* PointLights;
};

[[vk::push_constant]]
public ConstantBuffer<PathTracingInput> Input;

}
