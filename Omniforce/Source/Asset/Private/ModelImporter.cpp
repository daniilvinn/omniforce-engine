#include <Foundation/Common.h>
#include <Asset/Importers/ModelImporter.h>

#include <Core/Utils.h>
#include <Asset/AssetManager.h>
#include <Asset/MeshPreprocessor.h>
#include <Asset/AssetCompressor.h>
#include <Asset/VertexQuantizer.h>
#include <Asset/Material.h>
#include <Asset/Model.h>
#include <Asset/Importers/MaterialImporter.h>
#include <Asset/Importers/ImageImporter.h>
#include <Asset/VirtualMeshBuilder.h>
#include <Renderer/Mesh.h>
#include <Renderer/Image.h>
#include <Renderer/AccelerationStructure.h>
#include <Threading/JobSystem.h>
#include <Platform/Vulkan/Private/VulkanMemoryAllocator.h>
#include <Core/BitStream.h>
#include <Core/EngineConfig.h>

#include <map>
#include <atomic>

#include <glm/gtc/type_precision.hpp>
#include <glm/glm.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <spdlog/fmt/fmt.h>
#include <taskflow/taskflow.hpp>

namespace Omni {

	// just an alias for readability
	namespace ftf = fastgltf;

	static EngineConfigValue<bool> s_BuildVirtualGeometry("Renderer.UseVirtualGeometry", "Enables virtual geometry raster renderer");

	AssetHandle ModelImporter::Import(std::filesystem::path path)
	{
		// Setup timer
		Timer timer;
		OMNIFORCE_CORE_INFO("Importing model \"{}\"...", path.string());

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
					Ref<Mesh> mesh;
					AABB lod0_aabb = {};
					ftf::Material& ftf_material = ftf_asset.materials[primitive.materialIndex.value()];

					auto mesh_process_task = subflow.emplace([&, this]() {
						ProcessMeshData(&mesh, &lod0_aabb, &vertex_data, &index_data, vertex_stride, attribute_metadata_table, ftf_material, &mtx);
						OMNIFORCE_CORE_TRACE("[{}/{}] Loaded mesh: {}", ++mesh_load_progress_counter, ftf_asset.meshes.size(), ftf_mesh.name);
					}).succeed(attribute_read_task);

					// 4. Process material data - load textures, generate mip-maps, compress. Also copies scalar material properties
					//    Material processing requires additional steps in order to make sure that no duplicates will be created.
					Ref<Material> material;

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
		return AssetManager::Get()->RegisterAsset(Model::Create(&g_PersistentAllocator, submeshes));
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
		//if (!material->pbrData.baseColorTexture.has_value()) {
		//	OMNIFORCE_CORE_WARNING("One of the submeshes \"{}\" has no PBR material. Skipping submesh", mesh->name);
		//	OMNIFORCE_CUSTOM_LOGGER_WARN("OmniEditor", "One of the submeshes has no PBR material. Skipping submesh");
		//	return true;
		//}
		// Check if mesh data is indexed
		if (!primitive->indicesAccessor.has_value()) {
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
		for (auto attribute : primitive->attributes) {
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

	Ptr<RTAccelerationStructure> ModelImporter::BuildAccelerationStructure(
		const std::vector<byte>& vertex_data, 
		const std::vector<uint32>& index_data, 
		uint32 vertex_stride, 
		MaterialDomain domain
	)
	{
		DeviceBufferSpecification vertex_buffer_spec = {};
		vertex_buffer_spec.size = vertex_data.size();
		vertex_buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		vertex_buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
		vertex_buffer_spec.heap = DeviceBufferMemoryHeap::HOST;
		vertex_buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_INPUT;

		Ref<DeviceBuffer> vertex_buffer = DeviceBuffer::Create(&g_TransientAllocator, vertex_buffer_spec, (void*)vertex_data.data(), vertex_data.size());

		DeviceBufferSpecification index_buffer_spec = {};
		index_buffer_spec.size = index_data.size() * sizeof(uint32);
		index_buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		index_buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
		index_buffer_spec.heap = DeviceBufferMemoryHeap::HOST;
		index_buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_INPUT;

		Ref<DeviceBuffer> index_buffer = DeviceBuffer::Create(&g_TransientAllocator, index_buffer_spec, (void*)index_data.data(), index_data.size() * sizeof(uint32));

		BLASBuildInfo blas_build_info = {};
		blas_build_info.geometry = vertex_buffer;
		blas_build_info.indices = index_buffer;
		blas_build_info.vertex_count = vertex_data.size() / vertex_stride;
		blas_build_info.index_count = index_data.size();
		blas_build_info.vertex_stride = vertex_stride;
		blas_build_info.domain = domain;

		Ptr<RTAccelerationStructure> as = RTAccelerationStructure::Create(&g_PersistentAllocator, blas_build_info);

		return as;
	}

	GeometryLayoutTable ModelImporter::BuildLayoutTable(uint32 vertex_stride, const VertexAttributeMetadataTable& vertex_metadata)
	{
		GeometryLayoutTable layout = {};
		layout.Stride = vertex_stride - sizeof(glm::vec3);
		
		for (auto& metadata_entry : vertex_metadata) {
			if (metadata_entry.first.find("TEXCOORD") != std::string::npos) {
				layout.AttributeMask.HasUV = true;
				layout.AttributeMask.UVCount++;

				const std::string& entry_key = metadata_entry.first;
				
				std::string UV_index_string = entry_key.substr(entry_key.length() - 1);
				uint32 UV_index = std::atoi(UV_index_string.c_str());

				layout.Offsets.UV[UV_index] = metadata_entry.second - sizeof(glm::vec3);
			}
			if (metadata_entry.first.find("NORMAL") != std::string::npos) {
				layout.AttributeMask.HasNormals = true;
				layout.Offsets.Normal = metadata_entry.second - sizeof(glm::vec3);
			}
			if (metadata_entry.first.find("TANGENT") != std::string::npos) {
				layout.AttributeMask.HasTangents = true;
				layout.Offsets.Tangent = metadata_entry.second - sizeof(glm::vec3);
			}
			if (metadata_entry.first.find("COLOR") != std::string::npos) {
				layout.AttributeMask.HasColor = true;
				layout.Offsets.Color = metadata_entry.second - sizeof(glm::vec3);
			}
		}

		return layout;
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
				ftf::iterateAccessorWithIndex<glm::vec4>(*asset, accessor,
					[&](const glm::vec4 value, std::size_t idx) {
						const glm::u8vec4 quantized_color = quantizer.QuantizeColor(value);
						memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &quantized_color, sizeof(quantized_color));
					}
				);
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

	void ModelImporter::ProcessMeshData(
		Ref<Mesh>* out_mesh,
		AABB* out_lod0_aabb,
		const std::vector<byte>* vertex_data,
		const std::vector<uint32>* index_data,
		uint32 vertex_stride,
		const VertexAttributeMetadataTable& vertex_metadata,
		ftf::Material& material,
		std::shared_mutex* mtx
	) {
		// Init crucial data
		MeshData mesh_data;

		AABB lod0_aabb = {};
		VirtualMesh vmesh = {};

		MeshPreprocessor mesh_preprocessor = {};

		// Prepare storage for ray tracing data
		std::vector<byte> as_position_data(vertex_data->size() / vertex_stride * sizeof(glm::vec3));
		mesh_data.ray_tracing.attributes.resize(vertex_data->size() / vertex_stride * (vertex_stride - sizeof(glm::vec3)));
		mesh_data.ray_tracing.indices = *index_data;
		mesh_data.ray_tracing.layout = BuildLayoutTable(vertex_stride, vertex_metadata);

		// Split lod 0 data for ray tracing
		mesh_preprocessor.SplitVertexData(&as_position_data, &mesh_data.ray_tracing.attributes, vertex_data, vertex_stride);

		// Build acceleration structure
		MaterialDomain material_domain = MaterialDomain::NONE;

		switch (material.alphaMode) {
			case ftf::AlphaMode::Opaque:
				material_domain = MaterialDomain::OPAQUE;
				break;
			case ftf::AlphaMode::Mask:
				material_domain = MaterialDomain::MASKED;
				break;
			case ftf::AlphaMode::Blend:
				material_domain = MaterialDomain::TRANSMISSIVE;
				break;
			default:
				material_domain = MaterialDomain::NONE;
				break;
		};

		mesh_data.acceleration_structure = BuildAccelerationStructure(
			as_position_data, 
			*index_data, 
			sizeof(glm::vec3), 
			material_domain
		);

		// Check config for virtual geometry usage
		mesh_data.virtual_geometry.use = s_BuildVirtualGeometry.Get();
		if (!s_BuildVirtualGeometry.Get()) {
			std::lock_guard lock(*mtx);
			*out_mesh = Mesh::Create(
				&g_PersistentAllocator,
				mesh_data,
				lod0_aabb
			);

			AssetManager::Get()->RegisterAsset(*out_mesh);
			return;
		}

		// Optimize mesh (remove redundant vertices, optimize for vertex cache etc.)
		std::vector<byte> optimized_vertices;
		std::vector<uint32> optimized_indices;

		mesh_preprocessor.OptimizeMesh(&optimized_vertices, &optimized_indices, vertex_data, index_data, vertex_stride);

		VirtualMeshBuilder vmesh_builder = {};

		vmesh = vmesh_builder.BuildClusterGraph(optimized_vertices, optimized_indices, vertex_stride, vertex_metadata);

		OMNIFORCE_ASSERT_TAGGED(vmesh.meshlets.size(), "No virtual mesh clusters generated");
		OMNIFORCE_ASSERT_TAGGED(vmesh.indices.size() >= 3, "No virtual mesh indices generated");
		OMNIFORCE_ASSERT_TAGGED(vmesh.local_indices.size() >= 3, "No virtual mesh local indices generated");


		// Remap vertices to get rid of generated index buffer after generation of meshlets
		std::vector<byte> remapped_vertices(vmesh.indices.size() * vertex_stride);
		mesh_preprocessor.RemapVertices(&remapped_vertices, &optimized_vertices, vertex_stride, &vmesh.indices);

		// Split vertex data into two data streams: geometry and attributes.
		// It is an optimization used for depth-prepass and shadow maps rendering to speed up data reads.
		std::vector<glm::vec3> deinterleaved_vertex_data(remapped_vertices.size() / vertex_stride);
		mesh_data.virtual_geometry.attributes.resize(remapped_vertices.size() / vertex_stride * (vertex_stride - sizeof(glm::vec3)));
		mesh_preprocessor.SplitVertexData(&deinterleaved_vertex_data, &mesh_data.virtual_geometry.attributes, &remapped_vertices, vertex_stride);

		// Generate mesh bounds
		Bounds mesh_bounds = mesh_preprocessor.GenerateMeshBounds(&deinterleaved_vertex_data);

		// Copy data
		mesh_data.virtual_geometry.meshlets = vmesh.meshlets;
		mesh_data.virtual_geometry.local_indices = vmesh.local_indices;
		mesh_data.virtual_geometry.cull_data = vmesh.cull_bounds;
		mesh_data.virtual_geometry.bounding_sphere = mesh_bounds.sphere;

		// test on OOB
		for (auto& meshlet : vmesh.meshlets) {
			OMNIFORCE_ASSERT_TAGGED(meshlet.vertex_offset + meshlet.metadata.vertex_count <= deinterleaved_vertex_data.size(), "OOB");

			OMNIFORCE_ASSERT_TAGGED(meshlet.triangle_offset + meshlet.metadata.triangle_count <= vmesh.local_indices.size(), "OOB");

			for (uint32 local_index_idx = 0; local_index_idx < meshlet.metadata.triangle_count * 3; local_index_idx++) {
				OMNIFORCE_ASSERT_TAGGED(meshlet.triangle_offset + local_index_idx < vmesh.local_indices.size(), "OOB");
				uint32 local_index = vmesh.local_indices[meshlet.triangle_offset + local_index_idx];
				OMNIFORCE_ASSERT_TAGGED(local_index < meshlet.metadata.vertex_count, "OOB");
			}
		}

		// If i == 0, save generated mesh AABB, which will be used for LOD selection on runtime
		lod0_aabb = mesh_bounds.aabb;

		// Quantize vertex positions
		VertexDataQuantizer quantizer;
		const uint32 vertex_bitrate = quantizer.ComputeOptimalVertexBitrate(lod0_aabb);
		mesh_data.virtual_geometry.quantization_grid_size = vertex_bitrate;
		uint32 mesh_bitrate = quantizer.ComputeMeshBitrate(vertex_bitrate, lod0_aabb);
		uint32 vertex_bitstream_bit_size = deinterleaved_vertex_data.size() * mesh_bitrate * 3;
		uint32 grid_size = 1u << vertex_bitrate;

		// byte size is aligned by 4 bytes. So if we have 17 bits worth of data, we create a 4 bytes long bit stream. 
		// if we have 67 bits worth of data, we create 12 bytes long bit stream
		// We reserve worse case memory size
		Ptr<BitStream> vertex_stream = CreatePtr<BitStream>(&g_PersistentAllocator, (vertex_bitstream_bit_size + BitStream::StorageTypeBitSize - 1) / BitStream::StorageTypeBitSize * 4u);
		uint32 meshlet_idx = 0;

		for (auto& meshlet_bounds : mesh_data.virtual_geometry.cull_data) {
			uint32 meshlet_bitrate = std::clamp((uint32)std::ceil(std::log2(meshlet_bounds.vis_culling_sphere.radius * 2 * grid_size)), 1u, BitStream::StorageTypeBitSize); // we need diameter of a sphere, not radius

			OMNIFORCE_ASSERT_TAGGED(meshlet_bitrate <= BitStream::StorageTypeBitSize, "Bit stream overflow");

			RenderableMeshlet& meshlet = mesh_data.virtual_geometry.meshlets[meshlet_idx];

			meshlet.vertex_bit_offset = vertex_stream->GetNumBitsUsed();
			meshlet.metadata.bitrate = meshlet_bitrate;

			meshlet_bounds.vis_culling_sphere.center = glm::round(meshlet_bounds.vis_culling_sphere.center * float32(grid_size)) / float32(grid_size);

			uint32 base_vertex_offset = mesh_data.virtual_geometry.meshlets[meshlet_idx].vertex_offset;
			for (uint32 vertex_idx = 0; vertex_idx < meshlet.metadata.vertex_count; vertex_idx++) {
				for (uint32 vertex_channel = 0; vertex_channel < 3; vertex_channel++) {
					float32 original_vertex = deinterleaved_vertex_data[base_vertex_offset + vertex_idx][vertex_channel];
					float32 meshlet_space_value = original_vertex - meshlet_bounds.vis_culling_sphere.center[vertex_channel];

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

		mesh_data.virtual_geometry.geometry = std::move(vertex_stream);

		// Create mesh under mutex
		{
			std::lock_guard lock(*mtx);
			*out_mesh = Mesh::Create(
				&g_PersistentAllocator,
				mesh_data,
				lod0_aabb
			);
		}

		AssetManager::Get()->RegisterAsset(*out_mesh);
	}

	void ModelImporter::ProcessMaterialData(tf::Subflow& subflow, Ref<Material>* out_material, const ftf::Asset* asset, 
		const ftf::Material* material, const VertexAttributeMetadataTable* vertex_macro_table, std::shared_mutex* mtx)
	{
		MaterialImporter material_importer;
		auto& mat = *out_material;

		// Import mesh and join its task subflow used to load and process material properties
		mat = AssetManager::Get()->GetAsset<Material>(material_importer.Import(subflow, asset, material));
		subflow.join();

		// Add additional macros generated from vertex layout to generate correct shader variant
		// Also compute num uv channels in mesh
		uint32 num_uv_channels = 0;
		for (auto& metadata_entry : *vertex_macro_table) {
			if (metadata_entry.first.find("TEXCOORD") != std::string::npos) {
				if(num_uv_channels == 0)
					mat->AddShaderMacro("__OMNI_HAS_VERTEX_TEXCOORDS");
				num_uv_channels++;
			}
			mat->AddShaderMacro(fmt::format("__OMNI_HAS_VERTEX_{}", metadata_entry.first));
		}
		// Set macro of num of UV channels
		mat->AddShaderMacro("__OMNI_MESH_TEXCOORD_COUNT", std::to_string(num_uv_channels));

		// All data is gathered - compile material pipeline
		std::lock_guard lock(*mtx);

		if (s_BuildVirtualGeometry.Get()) {
			mat->CompilePipeline();
		}
	}
}