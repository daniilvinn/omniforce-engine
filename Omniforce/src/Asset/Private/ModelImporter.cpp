#include "../Importers/ModelImporter.h"

#include <Log/Logger.h>
#include <Core/Utils.h>
#include <Core/Timer.h>
#include <Asset/AssetManager.h>
#include <Asset/MeshPreprocessor.h>
#include <Asset/AssetCompressor.h>
#include <Asset/VertexQuantizer.h>
#include <Asset/Material.h>
#include <Asset/Model.h>
#include <Asset/Importers/MaterialImporter.h>
#include <Asset/Importers/ImageImporter.h>
#include <Renderer/Mesh.h>
#include <Renderer/Image.h>
#include <Threading/JobSystem.h>
#include <Platform/Vulkan/Private/VulkanMemoryAllocator.h>
#include <Core/BitStream.h>

#include <map>
#include <array>
#include <atomic>

#include <glm/gtc/type_precision.hpp>
#include <glm/glm.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <fmt/format.h>
#include <taskflow/taskflow.hpp>
#include <metis.h>

namespace Omni {

	// just an alias for readability
	namespace ftf = fastgltf;

	AssetHandle ModelImporter::Import(std::filesystem::path path)
	{
		// Setup timer
		Timer timer;
		OMNIFORCE_CORE_TRACE("Importing model \"{}\"...", path.string());

		// Init global data
		ftf::Asset ftf_asset;
		std::vector<MeshMaterialPair> submeshes;
		std::shared_mutex mtx;

		// Extract fastgltf::Asset
		ExtractAsset(&ftf_asset, path);

		// record task graph
		tf::Taskflow taskflow;
		rhumap<uint32, AssetHandle> material_table; // material index - AssetHandle table

		std::atomic_uint32_t mesh_load_progress_counter = 0;

		// Iterate over each mesh and spawn a task to load it
		for (auto& ftf_mesh : ftf_asset.meshes) {
			for (auto& primitive : ftf_mesh.primitives) {
				// Spawn conditional task to validate mesh (e.g. check it or its material is supported)
				auto primitive_validate_task = taskflow.emplace([&ftf_mesh, &primitive, &ftf_asset, this]() -> bool {
					ftf::Material& material = ftf_asset.materials[primitive.materialIndex.value()];
					return ValidateSubmesh(&ftf_mesh, &primitive, &material);
				});

				// If everything is ok after validation, then load it
				taskflow.emplace([&, this](tf::Subflow& subflow) {
					// 1. Read attribute metadata - find offsets, vertex stride
					VertexAttributeMetadataTable attribute_metadata_table = {};
					uint32 vertex_stride = 0;

					ReadVertexMetadata(&attribute_metadata_table, &vertex_stride, &ftf_asset, &primitive);

					// 2. Read vertex and index data. Record it in subflow so it can be executed in parallel to material loading
					std::vector<byte> vertex_data;
					std::vector<uint32> index_data;

					auto attribute_read_task = subflow.emplace([&, this]() {
						ReadVertexAttributes(&vertex_data, &index_data, &ftf_asset, &primitive, &attribute_metadata_table, vertex_stride);
					});

					// 3. Process mesh data - generate lods, optimize mesh, generate meshlets etc.
					Shared<Mesh> mesh = nullptr;
					AABB lod0_aabb = {};

					auto mesh_process_task = subflow.emplace([&, this]() {
						ProcessMeshData(&mesh, &lod0_aabb, &vertex_data, &index_data, vertex_stride, &mtx);
						OMNIFORCE_CORE_TRACE("[{}/{}] Loaded mesh: {}", ++mesh_load_progress_counter, ftf_asset.meshes.size(), ftf_mesh.name);
					}).succeed(attribute_read_task);

					// 4. Process material data - load textures, generate mip-maps, compress. Also copies scalar material properties
					//    Material processing requires additional steps in order to make sure that no duplicates will be created.
					const ftf::Material& ftf_material = ftf_asset.materials[primitive.materialIndex.value()];
					Shared<Material> material = nullptr;

					auto material_process_task = subflow.emplace([&, this](tf::Subflow& sf) {
						bool material_requires_processing = false;
						{
							std::lock_guard lock(mtx);
							if (!material_table.contains(primitive.materialIndex.value())) {
								material_table.emplace(primitive.materialIndex.value(), rh::hash<std::string>()(ftf_material.name.c_str()));
								material_requires_processing = true;
							}
						}

						if (material_requires_processing) {
							ProcessMaterialData(sf, &material, &ftf_asset, &ftf_material, &attribute_metadata_table, &mtx);
							OMNIFORCE_CORE_TRACE("Loaded material: {}", ftf_material.name);
						}
					});


					subflow.emplace([&]() {
						std::lock_guard lock(mtx);
						// Register mesh-material pair
						submeshes.push_back({ mesh->Handle, material_table.at(primitive.materialIndex.value()) });
					}).succeed(mesh_process_task, material_process_task);

					// Join so the stack doesn't get freed and we can safely use its memory
					subflow.join();
				}).succeed(primitive_validate_task);
			}
		}

		// Execute task graph
		JobSystem::GetExecutor()->run(taskflow).wait();

		// Write log
		OMNIFORCE_CORE_TRACE("Successfully imported model \"{}\". Time taken: {}s", path.string(), timer.ElapsedMilliseconds() / 1000.0f);

		// Create from loaded submeshes
		return AssetManager::Get()->RegisterAsset(Model::Create(submeshes));
	}

	void ModelImporter::ExtractAsset(ftf::Asset* asset, std::filesystem::path path)
	{
		// Allocate crucial fastgltf objects
		ftf::Parser gltf_parser;
		ftf::GltfDataBuffer data_buffer;

		// Try to load asset data
		if (!data_buffer.loadFromFile(path)) {
			OMNIFORCE_CORE_ERROR("Failed to load glTF model with path: {}. Aborting import.", path.string());
			return;
		}

		// Evaluate glTF type (glTF / GLB)
		ftf::GltfType source_type = ftf::determineGltfFileType(&data_buffer);

		// If invalid, abort loading
		if (source_type == ftf::GltfType::Invalid) {
			OMNIFORCE_CORE_ERROR("Failed to determine glTF file type with path: {}. Aborting import.", path.string());
			return;
		}

		// Setup options
		constexpr ftf::Options options = ftf::Options::DontRequireValidAssetMember |
			ftf::Options::LoadGLBBuffers | ftf::Options::LoadExternalBuffers |
			ftf::Options::LoadExternalImages | ftf::Options::GenerateMeshIndices | ftf::Options::DecomposeNodeMatrices;

		ftf::Expected<ftf::Asset> expected_asset(ftf::Error::None);

		// Call corresponding function to load either glTF or GLB asset
		source_type == ftf::GltfType::glTF ? expected_asset = gltf_parser.loadGltf(&data_buffer, path.parent_path(), options) :
			expected_asset = gltf_parser.loadGltfBinary(&data_buffer, path.parent_path(), options);

		// If errors are present, abort loading
		if (const auto error = expected_asset.error(); error != ftf::Error::None) {
			OMNIFORCE_CORE_ERROR("Failed to load asset source with path: {}. [{}]: {} Aborting import.", path.string(),
				ftf::getErrorName(error), ftf::getErrorMessage(error));
		}

		*asset = std::move(expected_asset.get());
	}

	bool ModelImporter::ValidateSubmesh(const ftf::Mesh* mesh, const ftf::Primitive* primitive, const ftf::Material* material)
	{
		// Check if material is PBR-compatible. Currently engine supports only PBR materials
		if (!material->pbrData.baseColorTexture.has_value()) {
			OMNIFORCE_CORE_WARNING("One of the submeshes \"{}\" has no PBR material. Skipping submesh", mesh->name);
			OMNIFORCE_CUSTOM_LOGGER_WARN("OmniEditor", "One of the submeshes has no PBR material. Skipping submesh");
			return true;
		}
		// Check if mesh data is indexed
		else if (!primitive->indicesAccessor.has_value()) {
			OMNIFORCE_CORE_ERROR("One of the submeshes has no indices. Unindexed meshes are not supported. Skipping submesh");
			OMNIFORCE_CUSTOM_LOGGER_ERROR("OmniEditor", "One of the submeshes has no indices. Unindexed meshes are not supported. Skipping submesh");
			return true;
		}
		// Check if topology is triangle list. Currently only triangle lists are supported
		else if (primitive->type != ftf::PrimitiveType::Triangles) {
			OMNIFORCE_CORE_ERROR("One of the submeshes primitive type is other than triangle list - currently only triangle list is supported. Skipping submesh");
			OMNIFORCE_CUSTOM_LOGGER_ERROR("OmniEditor", "One of the submeshes primitive type is other than triangle list - currently only triangle list is supported. Skipping submesh");
			return true;
		}
		return false;
	}

	void ModelImporter::ReadVertexMetadata(VertexAttributeMetadataTable* out_table, uint32* out_size, const ftf::Asset* asset, const ftf::Primitive* primitive)
	{
		uint32 attribute_stride = 12;

		// Iterate through attributes and add them to map, effectively sorting them
		for (auto& attribute : primitive->attributes) {
			// If on "POSIIION" attribute - skip iteration, since geometry is not considered as vertex attribute and is always at 0 offset
			if (attribute.first == "POSITION")
				continue;

			// Get fastgltf accesor
			auto& attrib_accessor = asset->accessors[attribute.second];
			out_table->emplace(attribute.first, 0);
		}

		// Now evaluate offsets for sorted attributes
		for (auto& attribute : *out_table) {
			attribute.second = attribute_stride;

			const auto& attribute_accessor = asset->accessors[primitive->findAttribute(attribute.first)->second];

			attribute_stride += VertexDataQuantizer::GetRuntimeAttributeSize(attribute.first);
		}
		*out_size = attribute_stride;
	}

	void ModelImporter::ReadVertexAttributes(std::vector<byte>* out_vertex_data, std::vector<uint32>* out_index_data, const ftf::Asset* asset, 
		const ftf::Primitive* primitive, const VertexAttributeMetadataTable* metadata, uint32 vertex_stride)
	{
		// Load indices
		{
			const auto& indices_accessor = asset->accessors[primitive->indicesAccessor.value()];
			out_index_data->resize(indices_accessor.count);
			{
				ftf::iterateAccessorWithIndex<uint32>(*asset, indices_accessor,
					[&](uint32 index, std::size_t idx) { (*out_index_data)[idx] = index; });
			}
		}

		// Load geometry
		VertexDataQuantizer quantizer;
		{
			const auto& vertices_accessor = asset->accessors[primitive->findAttribute("POSITION")->second];
			out_vertex_data->resize(vertices_accessor.count * vertex_stride);
			{
				ftf::iterateAccessorWithIndex<glm::vec3>(*asset, vertices_accessor,
					[&](const glm::vec3 position, std::size_t idx) { memcpy(out_vertex_data->data() + idx * vertex_stride, &position, sizeof position); });
			}
		}

		// Load attributes iterating through attribute metadata table to retrieve corresponding offset in data buffer
		for (auto& attrib : *metadata) {
			const auto& accessor = asset->accessors[primitive->findAttribute(attrib.first)->second];

			if (attrib.first.find("TEXCOORD") != std::string::npos) {
				ftf::iterateAccessorWithIndex<glm::vec2>(*asset, accessor,
					[&](const glm::vec2 value, std::size_t idx) { 
						const glm::u16vec2 quantized_uv = quantizer.QuantizeUV(value);
						memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &quantized_uv, sizeof(quantized_uv));
					}
				);
			}
			else if (attrib.first.find("NORMAL") != std::string::npos) {
				ftf::iterateAccessorWithIndex<glm::vec3>(*asset, accessor,
					[&](const glm::vec3 value, std::size_t idx) {
						const glm::u16vec2 quantized_normal = quantizer.QuantizeNormal(value);
						memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &quantized_normal, sizeof(quantized_normal));
					}
				);
			}
			else if (attrib.first.find("TANGENT") != std::string::npos) {
				ftf::iterateAccessorWithIndex<glm::vec4>(*asset, accessor,
					[&](const glm::vec4 value, std::size_t idx) {
						const glm::u16vec2 quantized_tangent = quantizer.QuantizeTangent(value);
						memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &quantized_tangent, sizeof(quantized_tangent));
					}
				);
			}
			else if (attrib.first.find("COLOR") != std::string::npos) {
				OMNIFORCE_ASSERT_TAGGED(false, "Vertex colors are not implemented");
			}
			else if (attrib.first.find("JOINTS") != std::string::npos) {
				OMNIFORCE_ASSERT_TAGGED(false, "Skinned meshes are not supported");
			}
			else if (attrib.first.find("WEIGHTS") != std::string::npos) {
				OMNIFORCE_ASSERT_TAGGED(false, "Skinned meshes are not supported");
			}
			else {
				OMNIFORCE_ASSERT_TAGGED(false, "Unknown attribute");
			}
		}
	}

	void ModelImporter::ProcessMeshData(Shared<Mesh>* out_mesh, AABB* out_lod0_aabb, const std::vector<byte>* vertex_data, const std::vector<uint32>* index_data, uint32 vertex_stride, std::shared_mutex* mtx)
	{
		// Init crucial data
		std::array<MeshData, Mesh::OMNI_MAX_MESH_LOD_COUNT> mesh_lods = {};
		AABB lod0_aabb = {};

		// Process mesh data on per-LOD basis. It involves generating LOD index buffer, optimizing mesh, generating meshlets and remapping data
		for (uint32 i = 0; i < Mesh::OMNI_MAX_MESH_LOD_COUNT; i++) {
			MeshPreprocessor mesh_preprocessor = {};

			// Generate LOD. If i == 0, we basically generate a lod which is completely equal to source mesh
			std::vector<uint32> lod_indices;

			if (i != 0) {
				uint32 target_index_count = index_data->size() / (1 * glm::pow(4, i)) + (index_data->size() % 3);
				float target_error = 0.1f;

				// If simplifier was unable to generate LOD (returned index count is 0), try to generate with higher error until lod is generated
				while (lod_indices.size() == 0)
				{
					mesh_preprocessor.GenerateMeshLOD(
						&lod_indices,
						vertex_data,
						index_data,
						vertex_stride,
						target_index_count < 6 ? 6 : target_index_count, // clamp index count to 6 (plane)
						target_error, false
					);

					// HACK: Multiply target index count by 20% so we can generate generate something;
					// Best solution would be to stop generation of LODs and use amount of lods we were able to generate, instead of fixed amount of 4
					target_index_count *= std::clamp(6u, (uint32)(target_index_count * 1.2f), (uint32)index_data->size());
				}
			}
			else {
				lod_indices = *index_data;
			}

			OMNIFORCE_ASSERT_TAGGED(lod_indices.size(), "Mesh LOD generation failed. How did we return from loop above though?");

			// Optimize mesh (remove redundant vertices, optimize for vertex cache etc.)
			std::vector<byte> optimized_vertices;
			std::vector<uint32> optimized_indices;
			
			mesh_preprocessor.OptimizeMesh(&optimized_vertices, &optimized_indices, vertex_data, &lod_indices, vertex_stride);

			// Generate meshlets for mesh shading-based geometry processing.
			GeneratedMeshlets* generated_meshlets = mesh_preprocessor.GenerateMeshlets(&optimized_vertices, &optimized_indices, vertex_stride);
			
#define GENERATE_EDGE_VBO
#ifdef GENERATE_EDGE_VBO
			if (i == 0) {
				std::unordered_map<MeshletEdge, std::vector<uint32>, MeshletEdgeHasher> edges2Meshlets;
				std::unordered_map<uint32, std::vector<MeshletEdge>> meshlets2Edges; // probably could be a vector

				std::vector<uint32> geometry_only_indices;

				// A hack to satisfity meshopt with `index_count % 3 == 0` requirement
				uint32 ib_padding = 3 - (generated_meshlets->indices.size() % 3);
				generated_meshlets->indices.resize(generated_meshlets->indices.size() + ib_padding);

				mesh_preprocessor.GenerateShadowIndexBuffer(
					&geometry_only_indices,
					&generated_meshlets->indices,
					&optimized_vertices,
					12,
					vertex_stride
				);
				generated_meshlets->indices.resize(generated_meshlets->indices.size() - ib_padding);

				// per meshlet
				for (uint32 meshlet_idx = 0; meshlet_idx < generated_meshlets->meshlets.size(); meshlet_idx++) {
					auto& meshlet = generated_meshlets->meshlets[meshlet_idx];

					auto getVertexIndex = [&](uint32 index) {
						return geometry_only_indices[generated_meshlets->local_indices[index + meshlet.triangle_offset] + meshlet.vertex_offset];
					};

					// per triangle
					for (uint32 triangle_idx = 0; triangle_idx < meshlet.triangle_count; triangle_idx++) {
						// per edge
						for (uint32 edge_idx = 0; edge_idx < 3; edge_idx++) {
							MeshletEdge edge{ getVertexIndex(edge_idx + triangle_idx * 3), getVertexIndex(((edge_idx + 1) % 3) + triangle_idx * 3) };

							auto& edge_meshlets = edges2Meshlets[edge];
							if (std::find(edge_meshlets.begin(), edge_meshlets.end(), meshlet_idx) == edge_meshlets.end())
								edge_meshlets.push_back(meshlet_idx);
							meshlets2Edges[meshlet_idx].emplace_back(edge);
						}

					}
				}

				// remove edges which are not connected to 2 different meshlets
				std::erase_if(edges2Meshlets, [](const auto& pair) {
					return pair.second.size() <= 1;
				});

				meshlet_edges_vbo.reserve(edges2Meshlets.size() * 2);
				for (auto& edge_meshlets_pair : edges2Meshlets) {
					glm::vec3 v = {};

					// First vertex
					uint32 v_offset = edge_meshlets_pair.first.first * vertex_stride;
					memcpy(&v, optimized_vertices.data() + v_offset, sizeof glm::vec3);
					meshlet_edges_vbo.push_back(v);

					// Second vertex
					v_offset = edge_meshlets_pair.first.second * vertex_stride;
					memcpy(&v, optimized_vertices.data() + v_offset, sizeof glm::vec3);
					meshlet_edges_vbo.push_back(v);
				}

				idx_t vertexCount = generated_meshlets->meshlets.size(); // vertex count, from the point of view of METIS, where Meshlet = vertex
				idx_t ncon = 1; // only one constraint, minimum required by METIS
				idx_t nparts = generated_meshlets->meshlets.size() / 4; // groups of 4
				assert(nparts > 1); // must be at least 2 partitions
				idx_t options[METIS_NOPTIONS];
				METIS_SetDefaultOptions(options);

				// edge-cut, ie minimum cost betweens groups.
				options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
				options[METIS_OPTION_CCORDER] = 1; // identify connected components first

				// prepare storage for partition data
				// each vertex will get its partition index inside this vector after the edge-cut
				std::vector<idx_t> partition;
				partition.resize(vertexCount);

				// xadj
				std::vector<idx_t> xadjacency;
				xadjacency.reserve(vertexCount + 1);

				// adjncy
				std::vector<idx_t> edgeAdjacency;
				// weight of each edge
				std::vector<idx_t> edgeWeights;

				for (std::size_t meshletIndex = 0; meshletIndex < generated_meshlets->meshlets.size(); meshletIndex++) {
					std::size_t startIndexInEdgeAdjacency = edgeAdjacency.size();
					for (const auto& edge : meshlets2Edges[meshletIndex]) {
						auto connectionsIter = edges2Meshlets.find(edge);
						if (connectionsIter == edges2Meshlets.end()) {
							continue;
						}
						const auto& connections = connectionsIter->second;
						for (const auto& connectedMeshlet : connections) {
							if (connectedMeshlet != meshletIndex) {
								auto existingEdgeIter = std::find(edgeAdjacency.begin() + startIndexInEdgeAdjacency, edgeAdjacency.end(), connectedMeshlet);
								if (existingEdgeIter == edgeAdjacency.end()) {
									// first time we see this connection to the other meshlet
									edgeAdjacency.emplace_back(connectedMeshlet);
									edgeWeights.emplace_back(1);
								}
								else {
									// not the first time! increase number of times we encountered this meshlet
									std::ptrdiff_t d = std::distance(edgeAdjacency.begin(), existingEdgeIter);
									assert(d >= 0);
									assert(d < edgeWeights.size()); // "edgeWeights and edgeAdjacency do not have the same length?"
									edgeWeights[d]++;
								}
							}
						}
					}
					xadjacency.push_back(startIndexInEdgeAdjacency);
				}
				xadjacency.push_back(edgeAdjacency.size());

				idx_t edgeCut; // final cost of the cut found by METIS
				int result = METIS_PartGraphKway(
					&vertexCount,
					&ncon,
					xadjacency.data(),
					edgeAdjacency.data(),
					nullptr, /* vertex weights */
					nullptr, /* vertex size */
					edgeWeights.data(),
					&nparts,
					nullptr,
					nullptr,
					options,
					&edgeCut,
					partition.data()
				);

				OMNIFORCE_ASSERT_TAGGED(result == METIS_OK, "Virtual mesh builder: graph partition failed");

				std::vector<std::vector<uint32>> groups;
				groups.resize(nparts);
				for (std::size_t i = 0; i < generated_meshlets->meshlets.size(); i++) {
					idx_t partitionNumber = partition[i];
					groups[partitionNumber].push_back(i);
				}

			}
#endif

			// Remap vertices to get rid of generated index buffer after generation of meshlets
			std::vector<byte> remapped_vertices(generated_meshlets->indices.size() * vertex_stride);
			mesh_preprocessor.RemapVertices(&remapped_vertices, &optimized_vertices, vertex_stride, &generated_meshlets->indices);

			// Split vertex data into two data streams: geometry and attributes.
			// It is an optimization used for depth-prepass and shadow maps rendering to speed up data reads.
			std::vector<glm::vec3> deinterleaved_vertex_data(remapped_vertices.size() / vertex_stride);
			mesh_lods[i].attributes.resize(remapped_vertices.size() / vertex_stride * (vertex_stride - sizeof glm::vec3));
			mesh_preprocessor.SplitVertexData(&deinterleaved_vertex_data, &mesh_lods[i].attributes, &remapped_vertices, vertex_stride);

			// Generate mesh bounds
			Bounds mesh_bounds = mesh_preprocessor.GenerateMeshBounds(&deinterleaved_vertex_data);

			// Copy data
			mesh_lods[i].meshlets			= std::move(generated_meshlets->meshlets);
			mesh_lods[i].local_indices		= std::move(generated_meshlets->local_indices);
			mesh_lods[i].cull_data			= std::move(generated_meshlets->cull_bounds);
			mesh_lods[i].bounding_sphere	= mesh_bounds.sphere;

			delete generated_meshlets;

			// If i == 0, save generated mesh AABB, which will be used for LOD selection on runtime
			if (i == 0)
				lod0_aabb = mesh_bounds.aabb;

			// Quantize vertex positions
			VertexDataQuantizer quantizer;
			const uint32 vertex_bitrate = 8;
			mesh_lods[i].quantization_grid_size = vertex_bitrate;
			uint32 mesh_bitrate = quantizer.ComputeMeshBitrate(vertex_bitrate, lod0_aabb);
			uint32 vertex_bitstream_bit_size = deinterleaved_vertex_data.size() * mesh_bitrate * 3;
			uint32 grid_size = 1u << vertex_bitrate;

			// byte size is aligned by 4 bytes. So if we have 17 bits worth of data, we create a 4 bytes long bit stream. 
			// if we have 67 bits worth of data, we create 12 bytes long bit stream
			// We reserve worse case memory size
			Scope<BitStream> vertex_stream = std::make_unique<BitStream>((vertex_bitstream_bit_size + 31u) / 32u * 4u);
			uint32 meshlet_idx = 0;

			// Precision tests
			float32 min_error = FLT_MAX;
			float32 max_error = FLT_MIN;

			for (auto& meshlet_bounds : mesh_lods[i].cull_data) {
				uint32 meshlet_bitrate = std::clamp((uint32)std::ceil(std::log2(meshlet_bounds.radius * 2 * grid_size)), 1u, 32u); // we need diameter of a sphere, not radius

				RenderableMeshlet& meshlet = mesh_lods[i].meshlets[meshlet_idx];

				meshlet.vertex_bit_offset = vertex_stream->GetNumBitsUsed();
				meshlet.bitrate = meshlet_bitrate;

				meshlet_bounds.bounding_sphere_center = glm::round(meshlet_bounds.bounding_sphere_center * float32(grid_size)) / float32(grid_size);

				uint32 base_vertex_offset = mesh_lods[i].meshlets[meshlet_idx].vertex_offset;
				for (uint32 vertex_idx = 0; vertex_idx < meshlet.vertex_count; vertex_idx++) {
					for(uint32 vertex_channel = 0; vertex_channel < 3; vertex_channel++) {
						float32 original_vertex = deinterleaved_vertex_data[base_vertex_offset + vertex_idx][vertex_channel];
						float32 meshlet_space_value = original_vertex - meshlet_bounds.bounding_sphere_center[vertex_channel];


						uint32 value = quantizer.QuantizeVertexChannel(
							meshlet_space_value,
							vertex_bitrate,
							meshlet_bitrate
						);

						vertex_stream->Append(meshlet_bitrate, value);
					}
				}
				meshlet_idx++;
			}

			mesh_lods[i].geometry = std::move(vertex_stream);

		}

		// Create mesh under mutex
		{
			std::lock_guard lock(*mtx);
			*out_mesh = Mesh::Create(
				mesh_lods,
				lod0_aabb
			);
		}

		AssetManager::Get()->RegisterAsset(ShareAs<AssetBase>(*out_mesh));
	}

	void ModelImporter::ProcessMaterialData(tf::Subflow& subflow, Shared<Material>* out_material, const ftf::Asset* asset, 
		const ftf::Material* material, const VertexAttributeMetadataTable* vertex_macro_table, std::shared_mutex* mtx)
	{
		MaterialImporter material_importer;

		// Import mesh and join its task subflow used to load and process material properties
		*out_material = AssetManager::Get()->GetAsset<Material>(material_importer.Import(subflow, asset, material));
		subflow.join();

		// Add additional macros generated from vertex layout to generate correct shader variant
		// Also compute num uv channels in mesh
		uint32 num_uv_channels = 0;
		for (auto& metadata_entry : *vertex_macro_table) {
			if (metadata_entry.first.find("TEXCOORD") != std::string::npos) {
				if(num_uv_channels == 0)
					(*out_material)->AddShaderMacro("__OMNI_HAS_VERTEX_TEXCOORDS");
				num_uv_channels++;
			}
			(*out_material)->AddShaderMacro(fmt::format("__OMNI_HAS_VERTEX_{}", metadata_entry.first));
		}
		// Set macro of num of UV channels
		(*out_material)->AddShaderMacro("__OMNI_MESH_TEXCOORD_COUNT", std::to_string(num_uv_channels));

		// All data is gathered - compile material pipeline
		std::lock_guard lock(*mtx);
		(*out_material)->CompilePipeline();
	}
}