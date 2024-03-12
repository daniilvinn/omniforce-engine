#include "../MeshGenerator.h"

namespace Omni {

	std::pair<std::vector<glm::vec3>, std::vector<uint32>> Omni::MeshGenerator::GenerateIcosphere(uint32 subdivisions /*= 1*/)
	{
		float32 t = (1.0f + sqrt(5.0f)) / 2.0f;

		std::vector<glm::vec3> icosahedron_vertices = {
			glm::vec3{ -1,  t,  0 }, glm::vec3{ 1,  t,  0 }, glm::vec3{ -1, -t,  0 }, glm::vec3{ 1, -t,  0 },
			glm::vec3{  0, -1,  t }, glm::vec3{ 0,  1,  t }, glm::vec3{  0, -1, -t }, glm::vec3{ 0,  1, -t },
			glm::vec3{  t,  0, -1 }, glm::vec3{ t,  0,  1 }, glm::vec3{ -t,  0, -1 }, glm::vec3{-t,  0,  1 }
		};

		uint32 final_vertex_buffer_size = 12;

		for (int i = 0; i < subdivisions; i++)
			final_vertex_buffer_size *= 5;

		icosahedron_vertices.reserve(final_vertex_buffer_size);

		std::vector<uint32> icosahedron_indices{
			 0, 11, 5 ,  0,  5,  1 ,    0,  1,  7 ,   0, 7, 10 ,  0, 10, 11 ,
			 1,  5, 9 ,  5, 11,  4 ,   11, 10,  2 ,  10, 7,  6 ,  7,  1,  8 ,
			 3,  9, 4 ,  3,  4,  2 ,    3,  2,  6 ,   3, 6,  8 ,  3,  8,  9 ,
			 4,  9, 5 ,  2,  4, 11 ,    6,  2, 10 ,   8, 6,  7 ,  9,  8,  1 
		};

		for (uint32 sub = 0; sub < subdivisions; sub++) {
			std::vector<glm::vec3> new_vertices;
			std::vector<uint32> new_indices;


			for (uint32 i = 0; i < icosahedron_indices.size(); i += 3) {
				glm::vec3 v0 = icosahedron_vertices[icosahedron_indices[i]];
				glm::vec3 v1 = icosahedron_vertices[icosahedron_indices[i + 1]];
				glm::vec3 v2 = icosahedron_vertices[icosahedron_indices[i + 2]];

				glm::vec3 v01 = glm::normalize((v0 + v1) / 2.0f);
				glm::vec3 v12 = glm::normalize((v1 + v2) / 2.0f);
				glm::vec3 v20 = glm::normalize((v2 + v0) / 2.0f);

				uint32 v01_index = new_vertices.size();
				uint32 v12_index = v01_index + 1;
				uint32 v20_index = v01_index + 2;

				new_vertices.push_back(v01);
				new_vertices.push_back(v12);
				new_vertices.push_back(v20);

				new_indices.push_back(icosahedron_indices[i]);
				new_indices.push_back(v01_index);
				new_indices.push_back(v20_index);
				new_indices.push_back( icosahedron_indices[i + 1]);
				new_indices.push_back(v12_index);
				new_indices.push_back(v01_index);
				new_indices.push_back(icosahedron_indices[i + 2]);
				new_indices.push_back(v20_index);
				new_indices.push_back(v12_index);
				new_indices.push_back(v01_index);
				new_indices.push_back(v12_index);
				new_indices.push_back(v20_index);
			}

#if 0
			for (auto& face : icosahedron_indices) {
				glm::vec3 v0 = icosahedron_vertices[face[0]];
				glm::vec3 v1 = icosahedron_vertices[face[1]];
				glm::vec3 v2 = icosahedron_vertices[face[2]];

				glm::vec3 v01 = glm::normalize((v0 + v1) / 2.0f);
				glm::vec3 v12 = glm::normalize((v1 + v2) / 2.0f);
				glm::vec3 v20 = glm::normalize((v2 + v0) / 2.0f);

				uint32 v01_index = new_vertices.size();
				uint32 v12_index = v01_index + 1;
				uint32 v20_index = v01_index + 2;

				new_vertices.push_back(v01);
				new_vertices.push_back(v12);
				new_vertices.push_back(v20);

				new_indices.push_back({ face[0], v01_index, v20_index });
				new_indices.push_back({ face[1], v12_index, v01_index });
				new_indices.push_back({ face[2], v20_index, v12_index });
				new_indices.push_back({ v01_index, v12_index, v20_index });
			}
#endif

			icosahedron_vertices = new_vertices;
			icosahedron_indices = new_indices;
		}



		return std::make_pair(icosahedron_vertices, icosahedron_indices);
	}

}