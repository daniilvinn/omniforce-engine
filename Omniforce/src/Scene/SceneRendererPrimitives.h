#pragma once

#include <Foundation/Types.h>

#include <Renderer/Mesh.h>

#include <glm/glm.hpp>

namespace Omni {

	struct DeviceCameraData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 view_proj;
		glm::vec3 position;
		Frustum frustum;
		glm::vec3 forward_vector;
	};

	struct DeviceMeshData {
		float32 lod_distance_multiplier;
		struct LOD {
			Sphere bounding_sphere;
			uint32 meshlet_count;
			uint64 geometry_bda;
			uint64 attributes_bda;
			uint64 meshlets_bda;
			uint64 micro_indices_bda;
			uint64 meshlets_cull_data_bda;
		} lods[Mesh::OMNI_MAX_MESH_LOD_COUNT];
	};

	struct TRS {
		glm::vec3 translation;
		glm::uvec2 rotation;
		glm::vec3 scale;
	};

	struct DeviceRenderableObject {
		TRS trs;
		const uint32 lod = 0; // computed on the device
		uint32 render_data_index;
		uint64 material_bda;
	};

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

}