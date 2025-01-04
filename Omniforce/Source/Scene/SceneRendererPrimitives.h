#pragma once

#include <Foundation/Common.h>

#include <Renderer/Mesh.h>

#include <glm/glm.hpp>

namespace Omni {

	// TODO: here some extreme hackery with alignment
	struct IndirectFrustumCullPassPushContants {
		uint64 camera_data_bda;
		uint64 mesh_data_bda;
		uint64 render_objects_data_bda;
		uint32 original_object_count;
		const uint32 _unused = 0;
		uint64 culled_objects_bda;
		uint64 output_buffer_bda;
	};

	struct PBRFullScreenPassPushConstants {
		uint64 camera_data_bda;
		uint64 point_lights_bda;
		uint32 positions_texture_index;
		uint32 base_color_texture_index;
		uint32 normal_texture_index;
		uint32 metallic_roughness_occlusion_texture_index;
		uint32 point_light_count;
		const uint32 _unused = 0;
	};

	struct ComputeClearPassPushConstants {
		uint64 out_bda;
		uint32 data_size;
		uint32 value;
	};

	// Just so I can do `sizeof()` with it
	struct SceneVisibleCluster {
		const uint32 instance_index = 0;
		const uint32 cluster_index = 0;
	};

}