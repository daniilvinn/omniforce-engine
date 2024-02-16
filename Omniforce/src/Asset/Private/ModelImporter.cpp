#include "../Importers/ModelImporter.h"

#include <Log/Logger.h>
#include <Core/Utils.h>
#include <Asset/AssetManager.h>
#include <Asset/MeshPreprocessor.h>
#include <Asset/AssetCompressor.h>
#include <Asset/Material.h>
#include <Asset/Model.h>
#include <Asset/Importers/MaterialImporter.h>
#include <Asset/Importers/ImageImporter.h>
#include <Renderer/Mesh.h>
#include <Renderer/Image.h>

#include <map>

#include <glm/gtc/type_precision.hpp>
#include <glm/glm.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <fmt/format.h>

namespace Omni {

	// just an alias for readability
	namespace ftf = fastgltf;

	AssetHandle ModelImporter::Import(std::filesystem::path path)
	{
		OMNIFORCE_CORE_TRACE("Importing model \"{}\"...", path.string());

		AssetManager* asset_manager = AssetManager::Get();

		ftf::Parser gltf_parser;
		ftf::GltfDataBuffer data_buffer;

		if (!data_buffer.loadFromFile(path)) {
			OMNIFORCE_CORE_ERROR("Failed to load glTF model with path: {}. Aborting import.", path.string());
			return 0;
		}

		ftf::GltfType source_type = ftf::determineGltfFileType(&data_buffer);

		if (source_type == ftf::GltfType::Invalid) {
			OMNIFORCE_CORE_ERROR("Failed to determine glTF file type with path: {}. Aborting import.", path.string());
			return 0;
		}

		constexpr ftf::Options options = ftf::Options::DontRequireValidAssetMember |
			ftf::Options::LoadGLBBuffers | ftf::Options::LoadExternalBuffers |
			ftf::Options::LoadExternalImages | ftf::Options::GenerateMeshIndices | ftf::Options::DecomposeNodeMatrices;

		ftf::Expected<ftf::Asset> expected_asset(ftf::Error::None);

		source_type == ftf::GltfType::glTF ? expected_asset = gltf_parser.loadGltf(&data_buffer, path.parent_path(), options) :
											 expected_asset = gltf_parser.loadGltfBinary(&data_buffer, path.parent_path(), options);

		if (const auto error = expected_asset.error(); error != ftf::Error::None) {
			OMNIFORCE_CORE_ERROR("Failed to load asset source with path: {}. [{}]: {}. Aborting import.", path.string(),
				ftf::getErrorName(error), ftf::getErrorMessage(error));

			return 0;
		}

		ftf::Asset& asset = expected_asset.get();

		if (asset.meshes.size() != 1) {
			OMNIFORCE_CORE_ERROR("Models with multiple meshes are currently not supported. Aborting import.");
		}

		std::vector<std::pair<AssetHandle, AssetHandle>> submeshes;
		submeshes.reserve(asset.meshes[0].primitives.size());

		for (auto& primitive : asset.meshes[0].primitives) {
			if (!primitive.indicesAccessor.has_value()) {
				OMNIFORCE_CORE_ERROR("Submesh of mesh \"{}\" has no indices. Unindexed meshes are not supported. Aborting import.", 
					asset.meshes[0].name);

				return 0;
			}

			std::vector<glm::vec3> geometry;
			std::vector<uint32> indices;
			std::vector<byte> attributes_data;
			std::map<std::string, uint8> present_attributes; // attrib name - attrib offset map
			std::vector<std::string> material_shader_macros;
			uint32 current_offset = 0;

			// Find offsets and present attributes
			for (auto& attribute : primitive.attributes) {
				if (attribute.first == "POSITION") continue;

				auto& attrib_accessor = asset.accessors[attribute.second];

				material_shader_macros.push_back(fmt::format("__OMNI_HAS_VERTEX_{}", attribute.first));

				uint8 attrib_size = fastgltf::getElementByteSize(attrib_accessor.type, attrib_accessor.componentType);
				attributes_data.resize(attributes_data.size() + attrib_size * attrib_accessor.count);
				present_attributes.emplace(attribute.first, current_offset);
				current_offset += attrib_size;
			}

			// Positions
			{
				const auto& vertices_accessor = asset.accessors[primitive.findAttribute("POSITION")->second];
				geometry.resize(vertices_accessor.count);
				{
					ftf::iterateAccessorWithIndex<glm::vec3>(asset, vertices_accessor,
						[&](const glm::vec3 position, std::size_t idx) { geometry[idx] = position; });
				}
			}

			// Indices
			{
				const auto& indices_accessor = asset.accessors[primitive.indicesAccessor.value()];
				indices.resize(indices_accessor.count);
				{
					ftf::iterateAccessorWithIndex<uint32>(asset, indices_accessor,
						[&](uint32 index, std::size_t idx) { indices[idx] = index; });
				}
			}

			// Fill attributes data. In lambdas, I use "current_offset" as vertex stride
			for (auto& attrib : present_attributes) {
				const auto& accessor = asset.accessors[primitive.findAttribute(attrib.first)->second];

				if (accessor.type == ftf::AccessorType::Vec2) {
					ftf::iterateAccessorWithIndex<glm::vec2>(asset, accessor,
						[&](const glm::vec2 value, std::size_t idx) { memcpy(attributes_data.data() + current_offset * idx + attrib.second, &value, sizeof(value)); });
				}
				else if (accessor.type == ftf::AccessorType::Vec3) {
					ftf::iterateAccessorWithIndex<glm::vec3>(asset, accessor,
						[&](const glm::vec3 value, std::size_t idx) { memcpy(attributes_data.data() + current_offset * idx + attrib.second, &value, sizeof(value)); });
				}
				else if (accessor.type == ftf::AccessorType::Vec4) {
					ftf::iterateAccessorWithIndex<glm::vec4>(asset, accessor,
						[&](const glm::vec4 value, std::size_t idx) { memcpy(attributes_data.data() + current_offset * idx + attrib.second, &value, sizeof(value)); });
				}

			}

			// Preprocess mesh before sending to the render device
			MeshPreprocessor mesh_preprocessor;
			mesh_preprocessor.OptimizeMesh(geometry, indices);

			GeneratedMeshlets* meshlets = mesh_preprocessor.GenerateMeshlets(geometry, indices);
			std::vector<glm::vec3> remapped_vertices(meshlets->indices.size());
			std::vector<byte> remapped_attributes(meshlets->indices.size() * current_offset); // use computed offset as stride

			uint32 idx = 0;
			for (auto& index : meshlets->indices) {
				remapped_vertices[idx] = geometry[index];
				memcpy(remapped_attributes.data() + current_offset * idx, attributes_data.data() + index * current_offset, current_offset);
				idx++;
			}

			Sphere bounding_sphere = mesh_preprocessor.GenerateBoundingSphere(geometry);

			Shared<Mesh> mesh = Mesh::Create(
				{ remapped_vertices.begin(), remapped_vertices.end() }, 
				{ remapped_attributes.begin(), remapped_attributes.end() }, 
				meshlets->meshlets, 
				meshlets->local_indices, 
				meshlets->cull_bounds,
				bounding_sphere
			);

			delete meshlets;

			// Process material
			auto& fastgltf_material = asset.materials[primitive.materialIndex.value()];

			MaterialImporter material_importer;
			Shared<Material> material = asset_manager->GetAsset<Material>(material_importer.Import(asset, fastgltf_material));
			for (auto& macro : material_shader_macros)
				material->AddShaderMacro(macro);

			material->CompilePipeline();

			submeshes.push_back({ asset_manager->RegisterAsset(ShareAs<AssetBase>(mesh)), material->Handle });
		}

		return asset_manager->RegisterAsset(Model::Create(submeshes));

		OMNIFORCE_CORE_TRACE("Successfully imported model \"{}\"", path.string());
		
	}

}