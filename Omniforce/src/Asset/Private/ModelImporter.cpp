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
#include <array>

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

		// Iterate over each mesh and spawn a task to load it
		for (auto& ftf_mesh : ftf_asset.meshes) {
			// Spawn conditional task to validate mesh (e.g. check it or its material is supported)
			auto primitive_validate_task = taskflow.emplace([&ftf_mesh, &ftf_asset, this]() -> bool {
				ftf::Material& material = ftf_asset.materials[ftf_mesh.primitives[0].materialIndex.value()];
				return ValidateSubmesh(&ftf_mesh, &material);
			});

			// If everything is ok after validation, then load it
			taskflow.emplace([&, this](tf::Subflow& subflow) {

				// 1. Read attribute metadata - find offsets, vertex stride
				VertexAttributeMetadataTable attribute_metadata_table = {};
				uint32 vertex_stride = 0;

				ReadVertexMetadata(&attribute_metadata_table, &vertex_stride, &ftf_asset, &ftf_mesh);

				// 2. Read vertex and index data. Record it in subflow so it can be executed in parallel to material loading
				std::vector<byte> vertex_data;
				std::vector<uint32> index_data;

				//auto attribute_read_task = subflow.emplace([&, this]() {
					ReadVertexAttributes(&vertex_data, &index_data, &ftf_asset, &ftf_mesh, &attribute_metadata_table, vertex_stride);
				//});

				// 3. Process mesh data - generate lods, optimize mesh, generate meshlets etc.
				Shared<Mesh> mesh = nullptr;
				AABB lod0_aabb = {};

				//auto mesh_process_task = subflow.emplace([&, this]() {
					ProcessMeshData(&mesh, &lod0_aabb, &vertex_data, &index_data, vertex_stride, &mtx);
				//}).succeed(attribute_read_task);

				// 4. Process material data - load textures, generate mip-maps, compress. Also copies scalar material properties
				//    Material processing requires additional steps in order to make sure that no duplicates will be created.
				const ftf::Material& ftf_material = ftf_asset.materials[ftf_mesh.primitives[0].materialIndex.value()];
				Shared<Material> material = nullptr;

				//auto material_process_task = subflow.emplace([&, this](tf::Subflow& sf) {
					if (!material_table.contains(ftf_mesh.primitives[0].materialIndex.value())) {
						material_table.emplace(ftf_mesh.primitives[0].materialIndex.value(), rh::hash<std::string>()(ftf_material.name.c_str()));
						ProcessMaterialData(subflow, &material, &ftf_asset, &ftf_material, &attribute_metadata_table, &mtx);
					}
				//});

				// Join so the stack doesn't get freed and we can safely use its memory
				subflow.join();

				// Register mesh-material pair
				submeshes.push_back({mesh->Handle, material_table.at(ftf_mesh.primitives[0].materialIndex.value())});
			}).succeed(primitive_validate_task);
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
			OMNIFORCE_CORE_ERROR("Failed to load asset source with path: {}. [{}]: {}. Aborting import.", path.string(),
				ftf::getErrorName(error), ftf::getErrorMessage(error));
		}

		*asset = std::move(expected_asset.get());
	}

	bool ModelImporter::ValidateSubmesh(const ftf::Mesh* mesh, const ftf::Material* material)
	{
		// Check if material is PBR-compatible. Currently engine supports only PBR materials
		if (!material->pbrData.baseColorTexture.has_value()) {
			OMNIFORCE_CORE_WARNING("One of the submeshes \"{}\" has no PBR material. Skipping submesh");
			OMNIFORCE_CUSTOM_LOGGER_WARN("OmniEditor", "One of the submeshes has no PBR material. Skipping submesh");
			return true;
		}
		// Check if mesh data is indexed
		else if (!mesh->primitives[0].indicesAccessor.has_value()) {
			OMNIFORCE_CORE_ERROR("One of the submeshes has no indices. Unindexed meshes are not supported. Skipping submesh");
			OMNIFORCE_CUSTOM_LOGGER_ERROR("OmniEditor", "One of the submeshes has no indices. Unindexed meshes are not supported. Skipping submesh");
			return true;
		}
		// Check if topology is triangle list. Currently only triangle lists are supported
		else if (mesh->primitives[0].type != ftf::PrimitiveType::Triangles) {
			OMNIFORCE_CORE_ERROR("One of the submeshes primitive type is other than triangle list - currently only triangle list is supported. Skipping submesh");
			OMNIFORCE_CUSTOM_LOGGER_ERROR("OmniEditor", "One of the submeshes primitive type is other than triangle list - currently only triangle list is supported. Skipping submesh");
			return true;
		}
		return false;
	}

	void ModelImporter::ReadVertexMetadata(VertexAttributeMetadataTable* out_table, uint32* out_stride, const ftf::Asset* asset, const ftf::Mesh* mesh)
	{
		auto& primitive = mesh->primitives[0];
		// Iterate through attributes and find their offsets
		uint32 attribute_stride = 12;
		for (auto& attribute : primitive.attributes) {
			// If on "POSIIION" attribute - skip iteration, since geometry is not considered as vertex attribute and is always at 0 offset
			if (attribute.first == "POSITION")
				continue;

			// Get fastgltf accesor
			auto& attrib_accessor = asset->accessors[attribute.second];

			// Calculate size. Emplace entry with key being attribute name and value being offset. Update stride
			uint8 attrib_size = fastgltf::getElementByteSize(attrib_accessor.type, attrib_accessor.componentType);
			out_table->emplace(attribute.first, attribute_stride);
			attribute_stride += attrib_size;
		}
		*out_stride = attribute_stride;
	}

	void ModelImporter::ReadVertexAttributes(std::vector<byte>* out_vertex_data, std::vector<uint32>* out_index_data, const ftf::Asset* asset, 
		const ftf::Mesh* mesh, const VertexAttributeMetadataTable* metadata, uint32 vertex_stride)
	{
		auto& primitive = mesh->primitives[0];
		// Load indices
		{
			const auto& indices_accessor = asset->accessors[primitive.indicesAccessor.value()];
			out_index_data->resize(indices_accessor.count);
			{
				ftf::iterateAccessorWithIndex<uint32>(*asset, indices_accessor,
					[&](uint32 index, std::size_t idx) { (*out_index_data)[idx] = index; });
			}
		}

		// Load geometry
		{
			const auto& vertices_accessor = asset->accessors[primitive.findAttribute("POSITION")->second];
			out_vertex_data->resize(vertices_accessor.count * vertex_stride);
			{
				ftf::iterateAccessorWithIndex<glm::vec3>(*asset, vertices_accessor,
					[&](const glm::vec3 position, std::size_t idx) { memcpy(out_vertex_data->data() + idx * vertex_stride, &position, sizeof position); });
			}
		}

		// Load attributes iterating through attribute metadata table to retrieve corresponding offset in data buffer
		for (auto& attrib : *metadata) {
			const auto& accessor = asset->accessors[primitive.findAttribute(attrib.first)->second];

			if (accessor.type == ftf::AccessorType::Vec2) {
				ftf::iterateAccessorWithIndex<glm::vec2>(*asset, accessor,
					[&](const glm::vec2 value, std::size_t idx) { memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &value, sizeof(value)); });
			}
			else if (accessor.type == ftf::AccessorType::Vec3) {
				ftf::iterateAccessorWithIndex<glm::vec3>(*asset, accessor,
					[&](const glm::vec3 value, std::size_t idx) { memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &value, sizeof(value)); });
			}
			else if (accessor.type == ftf::AccessorType::Vec4) {
				ftf::iterateAccessorWithIndex<glm::vec4>(*asset, accessor,
					[&](const glm::vec4 value, std::size_t idx) { memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &value, sizeof(value)); });
			}
		}
	}

	void ModelImporter::ProcessMeshData(Shared<Mesh>* out_mesh, AABB* out_lod0_aabb, const std::vector<byte>* vertex_data, const std::vector<uint32>* index_data, uint32 vertex_stride, std::shared_mutex* mtx)
	{
		// Init crucial data
		std::array<MeshData, 4> mesh_lods;
		AABB lod0_aabb;
		
		// Process mesh data on per-LOD basis. It involves generating LOD index buffer, optimizing mesh, generating meshlets and remapping data
		for (int32 i = 0; i < Mesh::OMNI_MAX_MESH_LOD_COUNT; i++) {
#if 0
			MeshPreprocessor mesh_preprocessor;
			uint8 deinterleaved_stride = vertex_stride - sizeof glm::vec3;

			// If i == 0, we are on LOD 0 (source mesh, so use original indices instead of generating LOD
			// Otherwise, generate LOD(i) index buffer and use it
			std::vector<uint32> lod_indices = i == 0 ? *index_data : mesh_preprocessor.GenerateLOD(
				*vertex_data,
				vertex_stride,
				*index_data,
				Utils::Align(index_data->size() / glm::pow(4, i), 3),
				0.1f + 0.1f * 3,
				false
			);

			// Optimize mesh using meshopt. 
			std::vector<byte> optimized_vertices;
			std::vector<uint32> optimized_indices;
			mesh_preprocessor.OptimizeMesh(&optimized_vertices, &optimized_indices, *vertex_data, lod_indices, vertex_stride);

			lod_indices.clear();

			// Split vertex data into two buffers. First buffer has geometry data only, second has attribute data
			// It is optimization used for shadow maps generation and depth prepass.
			std::vector<glm::vec3> deinterleaved_geometry(optimized_vertices.size() / vertex_stride);
			std::vector<byte> deinterleaved_attribs(optimized_vertices.size() / vertex_stride * deinterleaved_stride);
			mesh_preprocessor.SplitVertexData(&deinterleaved_geometry, &deinterleaved_attribs, optimized_vertices, vertex_stride);

			// Generate meshlets. My engine only supports rendering with mesh shading-based geometry processing.
			GeneratedMeshlets* meshlets = mesh_preprocessor.GenerateMeshlets(deinterleaved_geometry, optimized_indices);

			optimized_vertices.clear();
			optimized_indices.clear();

			// Remap vertices to get rid of remap table generated by meshopt. It is slightly (on tested models its value is about 20%) 
			// increases memory usage on per-LOD basis, but it eliminates need of index buffer, hence removes one indirection level.
			// Mesh shading still involves using index buffers, but they are called "micro index buffer" and operate on single meshlet, not a whole mesh
			std::vector<glm::vec3> remapped_vertices(meshlets->indices.size());
			std::vector<byte> remapped_attributes(meshlets->indices.size() * deinterleaved_stride);

			uint32 idx = 0;
			for (auto& index : meshlets->indices) {
				remapped_vertices[idx] = deinterleaved_geometry[index];
				memcpy(remapped_attributes.data() + deinterleaved_stride * idx, deinterleaved_attribs.data() + index * deinterleaved_stride, deinterleaved_stride);

				idx++;
			}

			// Compute LOD bounding sphere and AABB. If i == 0, we are at LOD 0 and we need to save AABB, which will be used for LOD selection in compute shaders
			Bounds mesh_bounds = mesh_preprocessor.GenerateMeshBounds(deinterleaved_geometry);
			if (i == 0)
				lod0_aabb = mesh_bounds.aabb;

			// Register data
			mesh_lods[i] = {
				std::move(remapped_vertices),
				std::move(remapped_attributes),
				std::move(meshlets->meshlets),
				std::move(meshlets->local_indices),
				std::move(meshlets->cull_bounds),
				std::move(mesh_bounds.sphere)
			};

			delete meshlets;
#endif
			MeshPreprocessor mesh_preprocessor = {};

			std::vector<uint32> lod_indices;

			if (i == 0)
				lod_indices = *index_data;
			else
				mesh_preprocessor.GenerateMeshLOD(&lod_indices, vertex_data, index_data, vertex_stride, Utils::Align(index_data->size() / glm::pow(i, 3), 3));

			std::vector<byte> optimized_vertices;
			std::vector<uint32> optimized_indices;

			mesh_preprocessor.OptimizeMesh(&optimized_vertices, &optimized_indices, vertex_data, &lod_indices, vertex_stride);

			lod_indices.clear(); // just reduce peak memory

			GeneratedMeshlets* generated_meshlets = mesh_preprocessor.GenerateMeshlets(&optimized_vertices, &optimized_indices, vertex_stride);

			std::vector<byte> remapped_vertices(generated_meshlets->indices.size() * vertex_stride);
			mesh_preprocessor.RemapVertices(&remapped_vertices, vertex_data, vertex_stride, &generated_meshlets->indices);

			mesh_preprocessor.SplitVertexData(&mesh_lods[i].geometry, &mesh_lods[i].attributes, &remapped_vertices, vertex_stride);

			Bounds mesh_bounds = mesh_preprocessor.GenerateMeshBounds(&mesh_lods[i].geometry);

			mesh_lods[i].meshlets			= std::move(generated_meshlets->meshlets);
			mesh_lods[i].local_indices		= std::move(generated_meshlets->local_indices);
			mesh_lods[i].cull_data			= std::move(generated_meshlets->cull_bounds);
			mesh_lods[i].bounding_sphere	= mesh_bounds.sphere;

			if (i == 0)
				lod0_aabb = mesh_bounds.aabb;
		}

		// Create mesh under mutex
		mtx->lock();
		*out_mesh = Mesh::Create(
			mesh_lods,
			lod0_aabb
		);
		mtx->unlock();
	}

	void ModelImporter::ProcessMaterialData(tf::Subflow& properties_load_subflow, Shared<Material>* out_material, const ftf::Asset* asset, 
		const ftf::Material* material, const VertexAttributeMetadataTable* vertex_macro_table, std::shared_mutex* mtx)
	{
		MaterialImporter material_importer;

		// Import mesh and join its task subflow used to load and process material properties
		material_importer.Import(properties_load_subflow, *asset, *material);
		properties_load_subflow.join();

		// Add additional macros generated from vertex layout to generate correct shader variant
		for (auto& metadata_entry : *vertex_macro_table)
			(*out_material)->AddShaderMacro(fmt::format("__OMNI_HAS_VERTEX_{}", metadata_entry.first));

		// All data is gathered - compile material pipeline
		(*out_material)->CompilePipeline();
	}
}