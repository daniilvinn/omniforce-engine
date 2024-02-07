#pragma once

#include <glm/glm.hpp>
#include <Foundation/Types.h>

namespace Omni {

	struct DeviceCameraData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 view_proj;
		glm::vec3 position;
	};

	struct DeviceMeshData {
		Sphere bounding_sphere;
		uint32 transform_id;
		uint64 geometry_bda;
		uint64 attributes_bda;
		uint64 meshlets_bda;
		uint64 micro_indices_bda;
		uint64 meshlets_cull_data_bda;
	};

}