#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Core/Utils.h>

#include <glm/gtc/type_precision.hpp>

namespace Omni {

	struct OMNIFORCE_API RenderableMeshlet {
		uint32 vertex_bit_offset = 0;
		uint32 vertex_offset = 0;
		uint32 triangle_offset = 0;
		struct {
			uint32 vertex_count : 7;
			uint32 triangle_count : 8;
			uint32 bitrate : 6;
		} metadata;
	};

	struct MeshletLODCullingData {
		Sphere sphere;
		Sphere parent_sphere = { glm::vec3(+INFINITY), +INFINITY };

		float32 error;
		float32 parent_error;
	};

	struct OMNIFORCE_API MeshletCullBounds {
		// Visibility culling data
		Sphere vis_culling_sphere;
		glm::vec3 cone_apex;
		glm::i8vec3 cone_axis;
		int8 cone_cutoff;

		// LOD culling data
		MeshletLODCullingData lod_culling;
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
			h = Omni::Utils::CombineHashes(h, edge.second);
			return h;
		}
	};

}