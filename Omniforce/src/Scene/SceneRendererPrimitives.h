#pragma once

#include <Foundation/Types.h>

#include <glm/glm.hpp>

namespace Omni {

	struct DeviceCameraData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 view_proj;
		glm::vec3 position;
		Frustum frustum;
	};

	struct DeviceMeshData {
		Sphere bounding_sphere;
		uint32 meshlet_count;
		uint64 geometry_bda;
		uint64 attributes_bda;
		uint64 meshlets_bda;
		uint64 micro_indices_bda;
		uint64 meshlets_cull_data_bda;
	};

	struct TRS {
		glm::vec3 translation;
		glm::uvec2 rotation;
		glm::vec3 scale;
	};

	struct DeviceRenderableObject {
		TRS trs;
		uint32 render_data_index;
		uint64 material_bda;
	};

}