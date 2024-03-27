#include "../Importers/ModelImporter.h"

#include <Log/Logger.h>
#include <Core/Utils.h>
#include <Core/Timer.h>
#include <Asset/AssetManager.h>
#include <Asset/MeshPreprocessor.h>
#include <Asset/AssetCompressor.h>
#include <Asset/Material.h>
#include <Asset/Model.h>
#include <Asset/Importers/MaterialImporter.h>
#include <Asset/Importers/ImageImporter.h>
#include <Renderer/Mesh.h>
#include <Renderer/Image.h>
#include <Threading/JobSystem.h>

#include <map>

#include <glm/gtc/type_precision.hpp>
#include <glm/glm.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <fmt/format.h>
#include <taskflow/taskflow.hpp>

namespace Omni {

	// just an alias for readability
	namespace ftf = fastgltf;

	AssetHandle ModelImporter::Import(std::filesystem::path path)
	{
		Timer timer;
		OMNIFORCE_CORE_TRACE("Importing model \"{}\"...", path.string());

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
			OMNIFORCE_CORE_ERROR("Models with multiple meshes are currently not supported. Aborting import. Or not?");
		}

		std::vector<std::pair<AssetHandle, AssetHandle>> submeshes;
		submeshes.reserve(asset.meshes.size());

		std::shared_mutex mtx;

		tf::Taskflow taskflow;
		for (auto& ftf_mesh : asset.meshes) {
			/*
			*  Spawn condition-producing task to early check if there any PBR data in material. If no, we can't render it
			*/
			tf::Task primitive_check_task = taskflow.emplace([&asset, ftf_mesh, &primitive = ftf_mesh.primitives[0]]() {
				auto& fastgltf_material = asset.materials[primitive.materialIndex.value()];

				if (!fastgltf_material.pbrData.baseColorTexture.has_value()) {
					OMNIFORCE_CORE_WARNING("One of the submeshes \"{}\" has no PBR material. Skipping submesh");
					return true;
				} else if (!primitive.indicesAccessor.has_value()) {
					OMNIFORCE_CORE_ERROR("One of the submeshes has no indices. Unindexed meshes are not supported. Skipping submesh",
						ftf_mesh.name);
					return true;
				} else if (primitive.type != ftf::PrimitiveType::Triangles) {
					OMNIFORCE_CORE_ERROR("One of the submeshes primitive type is other than triangle list - currently only triangle list is supported. Skipping submesh",
						ftf_mesh.name);
					return true;
				}
				return false;
			}).name("mesh primitive validation task");

			taskflow.emplace([&primitive = ftf_mesh.primitives[0], &asset, &mtx, &submeshes](tf::Subflow& subflow) {
				AssetManager* asset_manager = AssetManager::Get();
				std::vector<glm::vec3> geometry;
				std::vector<uint32> indices;
				std::vector<byte> attributes_data;
				std::map<std::string, uint8> present_attributes; // attrib name - attrib offset map
				std::vector<std::string> material_shader_macros;
				uint32 current_offset = 0;

				std::vector<byte> vertex_data;

				Shared<Material> material = nullptr;
				Shared<Mesh> mesh = nullptr;
				
				/*
				*  Spawn condition-producing task to prevent further execution if mesh is unsupported
				*/
				tf::Task attribute_metadata_eval_task = subflow.emplace([&]() {
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
				}).name("attribute metadata eval task");

				tf::Task attribute_load_task = subflow.emplace([&]() {
					// Indices
					{
						const auto& indices_accessor = asset.accessors[primitive.indicesAccessor.value()];
						indices.resize(indices_accessor.count);
						{
							ftf::iterateAccessorWithIndex<uint32>(asset, indices_accessor,
								[&](uint32 index, std::size_t idx) { indices[idx] = index; });
						}
					}

					// Positions
					{
						const auto& vertices_accessor = asset.accessors[primitive.findAttribute("POSITION")->second];
						vertex_data.resize(vertices_accessor.count * (sizeof glm::vec3 + current_offset));
						geometry.resize(vertices_accessor.count);
						{
							ftf::iterateAccessorWithIndex<glm::vec3>(asset, vertices_accessor,
								[&](const glm::vec3 position, std::size_t idx) { memcpy(vertex_data.data() + (idx * (current_offset + 12)), &position, sizeof position); });
						}
					}

					// Fill attributes data. In lambdas, I use "current_offset" as vertex stride
					for (auto& attrib : present_attributes) {
						const auto& accessor = asset.accessors[primitive.findAttribute(attrib.first)->second];

						if (accessor.type == ftf::AccessorType::Vec2) {
							ftf::iterateAccessorWithIndex<glm::vec2>(asset, accessor,
								[&](const glm::vec2 value, std::size_t idx) { memcpy(vertex_data.data() + (idx * (current_offset + 12)) + attrib.second + 12, &value, sizeof(value)); });
						}
						else if (accessor.type == ftf::AccessorType::Vec3) {
							ftf::iterateAccessorWithIndex<glm::vec3>(asset, accessor,
								[&](const glm::vec3 value, std::size_t idx) { memcpy(vertex_data.data() + (idx * (current_offset + 12)) + attrib.second + 12, &value, sizeof(value)); });
						}
						else if (accessor.type == ftf::AccessorType::Vec4) {
							ftf::iterateAccessorWithIndex<glm::vec4>(asset, accessor,
								[&](const glm::vec4 value, std::size_t idx) { memcpy(vertex_data.data() + (idx * (current_offset + 12)) + attrib.second + 12, &value, sizeof(value)); });
						}
					}
				}).succeed(attribute_metadata_eval_task);

				/*
				*   Spawn task to process imported mesh
				*/
				tf::Task mesh_processing_task = subflow.emplace([&]() {
					// Preprocess mesh before sending to the render device
					uint8 stride = current_offset;
					uint8 interleaved_stride = stride + 12; // count vertex position data as well, so +12
					MeshPreprocessor mesh_preprocessor;
					mesh_preprocessor.OptimizeMesh(vertex_data, indices, interleaved_stride);

					geometry.resize(vertex_data.size() / interleaved_stride);
					attributes_data.resize(vertex_data.size() / (interleaved_stride)*stride);
					// split data back
					for (int i = 0; i < vertex_data.size() / interleaved_stride; i++) {
						memcpy(geometry.data() + i, vertex_data.data() + i * interleaved_stride, 12);
						memcpy(attributes_data.data() + i * stride, vertex_data.data() + i * interleaved_stride + 12, stride);
					}

					// Generate meshlets for mesh shading
					GeneratedMeshlets* meshlets = mesh_preprocessor.GenerateMeshlets(geometry, indices);

					// Remap vertices to get rid of remap table generated by meshopt
					std::vector<glm::vec3> remapped_vertices(meshlets->indices.size());
					std::vector<byte> remapped_attributes(meshlets->indices.size() * stride);

					uint32 idx = 0;
					for (auto& index : meshlets->indices) {
						remapped_vertices[idx] = geometry[index];
						memcpy(remapped_attributes.data() + stride * idx, attributes_data.data() + index * stride, stride);

						idx++;
					}

					// Compute entire mesh bounding sphere
					Sphere bounding_sphere = mesh_preprocessor.GenerateBoundingSphere(geometry);

					mtx.lock();
					mesh = Mesh::Create(
						remapped_vertices,
						remapped_attributes,
						meshlets->meshlets,
						meshlets->local_indices,
						meshlets->cull_bounds,
						bounding_sphere
					);
					mtx.unlock();

					delete meshlets;
				}).name("mesh processing task").succeed(attribute_load_task);;

				/*
				*	Spawn task for material processing and loading
				*/
				tf::Task material_load_task = subflow.emplace([&, &ftf_material = asset.materials[primitive.materialIndex.value()]](tf::Subflow& material_load_subflow) {
					// Load material and compile pipeline if absent in pipeline library
					MaterialImporter material_importer;

					AssetHandle expected_id = rh::hash<std::string>()(ftf_material.name.c_str());
					if (asset_manager->HasAsset(expected_id)) {
						material = asset_manager->GetAsset<Material>(expected_id);
					}
					else {
						material = asset_manager->GetAsset<Material>(material_importer.Import(material_load_subflow, asset, ftf_material));
						material_load_subflow.join();

						for (auto& macro : material_shader_macros)
							material->AddShaderMacro(macro);

						material->CompilePipeline();
					}
				}).succeed(attribute_metadata_eval_task);

				/*
				*	Spawn task for final register of asset
				*/
				auto primitive_load_complete_task = subflow.emplace([&]() {
					mtx.lock();
					submeshes.push_back({ asset_manager->RegisterAsset(ShareAs<AssetBase>(mesh)), material->Handle });
					mtx.unlock();
				}).name("mesh register task").succeed(material_load_task, mesh_processing_task);

				subflow.join();
			}).name("mesh primitive load task").succeed(primitive_check_task);
		}

		JobSystem::GetExecutor()->run(taskflow).wait();

		OMNIFORCE_CORE_TRACE("Successfully imported model \"{}\". Time taken: {}s", path.string(), timer.ElapsedMilliseconds() / 1000.0f);

		return AssetManager::Get()->RegisterAsset(Model::Create(submeshes));
	}
}