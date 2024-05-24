#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Core/Utils.h>

namespace Omni {

	struct OMNIFORCE_API RenderableMeshlet {
		uint32 vertex_bit_offset;
		uint32 vertex_offset;
		uint32 triangle_offset;
		struct {
			uint32 vertex_count : 7;
			uint32 triangle_count : 8;
			uint32 bitrate : 6;
		} metadata;
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

}

namespace robin_hood {

	template<>
	struct hash<Omni::MeshletEdge> {
		size_t operator()(const Omni::MeshletEdge& edge) const {
			uint32_t h = hash<uint32_t>()(edge.first);
			Omni::Utils::CombineHashes(h, edge.second);
			return h;
		}
	};

}

namespace std {

	template<>
	struct hash<Omni::MeshletEdge> {
		size_t operator()(const Omni::MeshletEdge& edge) const {
			uint32_t h = hash<uint32_t>()(edge.first);
			Omni::Utils::CombineHashes(h, edge.second);
			return h;
		}
	};

}