#pragma once 

#include <Foundation/Common.h>

namespace Omni {

	struct META(ShaderExpose, Module = "RenderingGenerated") InstanceRenderData {
		Transform transform;
		uint32 geometry_data_id;
		uint64 material_address;
	};

}