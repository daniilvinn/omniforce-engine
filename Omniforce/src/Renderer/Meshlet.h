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

	template <class T>
	inline void hash_combine(T& seed, const T& v)
	{
		rh::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	struct MeshletEdge {
		explicit MeshletEdge(std::size_t a, std::size_t b) : first(std::min(a, b)), second(std::max(a, b)) {}

		bool operator==(const MeshletEdge& other) const = default;

		const uint32 first;
		const uint32 second;
	};

	struct MeshletEdgeHasher {
		uint32 operator()(const MeshletEdge& edge) const {
			uint32 h = edge.first;
			hash_combine(h, edge.second);
			return h;
		}
	};

}