#pragma once 

#include <Foundation/Common.h>

#include <CodeGeneration/Device/SpecialTypes.h>

namespace Omni {

	struct META(ShaderExpose, Module = "RenderingGenerated") InstanceRenderData {
		Transform transform;
		uint32 geometry_data_id;
		BDA<byte> material_address;
	};

	struct HostInstanceRenderData {
		Transform transform;
		AssetHandle mesh_handle;
		AssetHandle material_handle;
	};

}