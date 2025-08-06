#include <Foundation/Common.h>

#include <Rendering/PathTracing/PathTracingSettings.h>
#include <CodeGeneration/Device/SpecialTypes.h>
#include <CodeGeneration/Device/ViewData.h>
#include <CodeGeneration/Device/RenderObject.h>
#include <CodeGeneration/Device/MeshData.h>
#include <Scene/Lights.h>

namespace Omni {

	struct META(ShaderExpose, ShaderInput, Module = "PathTracingInput") PathTracingInput {
		BDA<ViewData> View;
		BDA<InstanceRenderData> Instances;
		BDA<GeometryMeshData> Meshes;
		uint32 RandomSeed;
		uint32 AccumulatedFrames;
		BDA<ScenePointLights> PointLights;
		BDA<PathTracingSettings> Settings;
	};
    
}