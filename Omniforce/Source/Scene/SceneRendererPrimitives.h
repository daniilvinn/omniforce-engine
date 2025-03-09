#pragma once

#include <Foundation/Common.h>

#include <Renderer/Mesh.h>
#include <CodeGeneration/Device/SpecialTypes.h>
#include <CodeGeneration/Device/ViewData.h>
#include <CodeGeneration/Device/MeshData.h>
#include <CodeGeneration/Device/RenderObject.h>
#include <Scene/Lights.h>
#include <Scene/Sprite.h>

#include <glm/glm.hpp>

namespace Omni {

	struct META(ShaderExpose, Module = "IndirectRendering") MeshShadingDrawParams {
		uint32 InstanceCount;
		ShaderRuntimeArray<glm::uvec3> DrawParams;
	};

	// TODO: here some extreme hackery with alignment
	struct META(ShaderExpose, ShaderInput, Module = "InstanceCullingInput") InstanceCullingInput {
		BDA<ViewData> View;
		BDA<GeometryMeshData> Meshes;
		BDA<InstanceRenderData> SourceInstances;
		uint32 SourceInstanceCount;
		BDA<InstanceRenderData> PostCullInstances;
		BDA<MeshShadingDrawParams> IndirectParams;
	};

	struct META(ShaderExpose, ShaderInput, Module = "PBRLightingInput") PBRLightingInput {
		BDA<ViewData> View;
		BDA<PointLight> PointLights;
		uint32 PointLightCount;
		uint32 PositionTextureID;
		uint32 ColorTextureID;
		uint32 NormalTextureID;
		uint32 MetallicRoughnessOcclusionTextureID;
	};

	struct ComputeClearPassPushConstants {
		uint64 out_bda;
		uint32 data_size;
		uint32 value;
	};

	// Just so I can do `sizeof()` with it
	struct META(ShaderExpose, Module = "RenderingGenerated") SceneVisibleCluster {
		uint32 InstanceIndex;
		uint32 ClusterIndex;
	};

	struct META(ShaderExpose, ShaderInput, Module = "ResolveVisibleMaterialsMaskInput") ResolveVisibleMaterialsMaskInput {
		BDA<InstanceRenderData> Instances;
		BDA<SceneVisibleCluster> VisibleClusters;
	};

	struct META(ShaderExpose, ShaderInput, Module = "SpriteRenderingInput") SpriteRenderingInput{
		BDA<ViewData> View;
		BDA<Sprite> Sprites;
		uint32 NumSprites;
	};

}