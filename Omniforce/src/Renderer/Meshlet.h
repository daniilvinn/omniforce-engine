#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

namespace Omni {

	struct OMNIFORCE_API RenderableMeshlet {
		uint32 vertex_bit_offset;
		uint32 vertex_offset;
		uint32 triangle_offset;
		uint32 vertex_count;
		uint32 triangle_count;
		uint32 bitrate;
	};

	struct OMNIFORCE_API MeshletCullBounds {
		glm::vec3 bounding_sphere_center;
		float32 radius;

		fvec3 cone_apex;
		fvec3 cone_axis;
		float32 cone_cutoff;
	};

}