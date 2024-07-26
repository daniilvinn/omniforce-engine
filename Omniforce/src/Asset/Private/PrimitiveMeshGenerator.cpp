// Implementation from https://schneide.blog/2016/07/15/generating-an-icosphere-in-c/

#include "../PrimitiveMeshGenerator.h"

#include <Log/Logger.h>

#include <array>

namespace Omni {

	using VertexList = std::vector<glm::vec3>;
	using TriangleList = std::vector<glm::uvec3>;
	using Lookup = std::map<std::pair<uint32, uint32>, uint32>;
	using uint32 = uint32;

	uint32 vertex_for_edge(Lookup& lookup, VertexList& vertices, uint32 first, uint32 second)
	{
		Lookup::key_type key(first, second);
		if (key.first > key.second)
			std::swap(key.first, key.second);

		auto inserted = lookup.insert({ key, (uint32)vertices.size() });
		if (inserted.second)
		{
			auto& edge0 = vertices[first];
			auto& edge1 = vertices[second];
			auto point = glm::normalize(edge0 + edge1);
			vertices.push_back(point);
		}

		return inserted.first->second;
	}

	TriangleList subdivide(VertexList& vertices,
		TriangleList triangles)
	{
		Lookup lookup;
		TriangleList result;

		for (auto&& each : triangles)
		{
			std::array<uint32, 3> mid;
			for (int edge = 0; edge < 3; ++edge)
			{
				mid[edge] = vertex_for_edge(lookup, vertices,
					each[edge], each[(edge + 1) % 3]);
			}

			result.push_back({ each[0], mid[0], mid[2] });
			result.push_back({ each[1], mid[1], mid[0] });
			result.push_back({ each[2], mid[2], mid[1] });
			result.push_back({ mid[0], mid[1], mid[2] });
		}

		return result;
	}

	std::pair<std::vector<glm::vec3>, std::vector<uint32>> Omni::PrimitiveMeshGenerator::GenerateIcosphere(uint32 subdivisions /*= 1*/)
	{
		const float32 X = .525731112119133606f;
		const float32 Z = .850650808352039932f;
		const float32 N = 0.f;

		VertexList icosahedron_vertices(12);
		icosahedron_vertices = {
			{-X,N,Z}, {X,N,Z}, {-X,N,-Z}, {X,N,-Z},
			{N,Z,X}, {N,Z,-X}, {N,-Z,X}, {N,-Z,-X},
			{Z,X,N}, {-Z,X, N}, {Z,-X,N}, {-Z,-X, N}
		};

		TriangleList icosahedron_indices =
		{
			{0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
			{8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
			{7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
			{6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
		};


		for (int i = 0; i < subdivisions; i++) {
			icosahedron_indices = subdivide(icosahedron_vertices, icosahedron_indices);
		}
		
		std::vector<uint32> indices(icosahedron_indices.size() * 3);
		memcpy(indices.data(), icosahedron_indices.data(), indices.size() * 4);

		return { icosahedron_vertices, indices };
	}

	std::pair<std::vector<glm::vec3>, std::vector<Omni::uint32>> PrimitiveMeshGenerator::GenerateCube()
	{
		std::vector<glm::vec3> vertices = {
			{-0.5f, -0.5f,  0.5f },
			{ 0.5f, -0.5f,  0.5f },
			{ 0.5f,  0.5f,  0.5f },
			{-0.5f,  0.5f,  0.5f },
			{-0.5f, -0.5f, -0.5f },
			{ 0.5f, -0.5f, -0.5f },
			{ 0.5f,  0.5f, -0.5f },
			{-0.5f,  0.5f, -0.5f }
		};

		std::vector<uint32> indices = {
			0, 1, 2,
			2, 3, 0,
			4, 0, 3,
			3, 7, 4,
			5, 4, 7,
			7, 6, 5,
			1, 5, 6,
			6, 2, 1,
			3, 2, 6,
			6, 7, 3,
			0, 4, 5,
			5, 1, 0
		};

		return { vertices, indices };
	}

}