#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Core/Utils.h>

namespace Omni {

	struct OMNIFORCE_API RenderableMeshlet {
		uint32 vertex_bit_offset;
		uint32 vertex_offset;
		uint32 triangle_offset;
		uint32 vertex_count;
		uint32 triangle_count;
		uint32 bitrate;
		uint32 group;
		uint32 lod = 0;
	};

	struct OMNIFORCE_API MeshletCullBounds {
		glm::vec3 bounding_sphere_center;
		float32 radius;

		fvec3 cone_apex;
		fvec3 cone_axis;
		float32 cone_cutoff;
	};

	using MeshClusterGroup = std::vector<uint32>;

	struct MeshletEdge {
		explicit MeshletEdge(uint32 a, uint32 b) : first(std::min(a, b)), second(std::max(a, b)) {}

		bool operator==(const MeshletEdge& other) const = default;

		const uint32 first;
		const uint32 second;
	};

	struct MeshletEdgeHasher {
		uint32 operator()(const MeshletEdge& edge) const {
			uint32 h = edge.first;
			Utils::CombineHashes(h, edge.second);
			return h;
		}
	};

}