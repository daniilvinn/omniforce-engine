#pragma once 

#include <Foundation/Common.h>

namespace Omni {

	struct META(ShaderExpose, Module = "RenderingGenerated") InstanceRenderData {
		Transform test;
		uint32 geometry_data_id;
		uint64 material_address;
	};

}