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

		// Debug data
		// TODO: remove this data after virtual geometry development is finished. 
		// DO NOT forget to remove it from GLSL meshlet declaration!
		// TODO: add shared source files so I can't forget to remove something when a structure is shared between C++ and GLSL
		uint32 group = 0;
		uint32 lod = 0;
	};

	struct OMNIFORCE_API MeshletCullBounds {
		Sphere vis_culling_sphere;

		glm::vec3 cone_apex;
		glm::i8vec3 cone_axis;
		int8 cone_cutoff;
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