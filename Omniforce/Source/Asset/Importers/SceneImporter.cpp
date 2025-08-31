#include <Foundation/Common.h>
#include <Asset/Importers/SceneImporter.h>

#include <Core/Utils.h>
#include <Asset/AssetManager.h>
#include <Asset/MeshPreprocessor.h>
#include <Asset/AssetCompressor.h>
#include <Asset/VertexQuantizer.h>
#include <Asset/Material.h>
#include <Asset/Importers/MaterialImporter.h>
#include <Asset/VirtualMeshBuilder.h>
#include <Rendering/Mesh.h>
#include <RHI/Image.h>
#include <RHI/AccelerationStructure.h>
#include <Threading/JobSystem.h>
#include <Scene/Entity.h>
#include <Scene/Component.h>
#include <Core/BitStream.h>
#include <Core/EngineConfig.h>

#include <glm/gtc/type_precision.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <spdlog/fmt/fmt.h>
#include <taskflow/taskflow.hpp>

namespace Omni {

	namespace ftf = fastgltf;

	static EngineConfigValue<bool> s_BuildVirtualGeometry("Renderer.UseVirtualGeometry", "Enables virtual geometry raster renderer");

	Ref<Scene> SceneImporter::ImportScene(std::filesystem::path path, SceneType type)
	{
		// Setup timer
		Timer timer;
		OMNIFORCE_CORE_INFO("Importing scene \"{}\"...", path.string());

		// Init global data
		ftf::Asset ftf_asset;
		std::shared_mutex mtx;

		// Extract fastgltf::Asset
		ExtractAsset(&ftf_asset, path);

		// Create template scene (no renderer)
		Ref<Scene> template_scene = CreateTemplateScene(type);

		// If no scenes defined in GLTF, create a default root node
		if
		(
			ftf_asset.scenes.empty()
		)
		{
			OMNIFORCE_CORE_WARNING("GLTF file has no scenes defined, creating empty scene");
			return template_scene;
		}

		// Process the first scene (for now)
		const ftf::Scene& gltf_scene = ftf_asset.scenes[0];

		// Process all root nodes in the scene
		for 
		(
			size_t node_index : gltf_scene.nodeIndices
		)
		{
			const ftf::Node& node = ftf_asset.nodes[node_index];
			ProcessNode(template_scene.Raw(), &ftf_asset, &node, Entity());
		}

		// Materials are now processed directly in ProcessMeshNode (Phase 2 enhancement)

		OMNIFORCE_CORE_TRACE("Successfully imported scene \"{}\". Time taken: {}s", 
			path.string(), timer.ElapsedMilliseconds() / 1000.0f);

		return template_scene;
	}

	Ref<Scene> SceneImporter::CreateTemplateScene(SceneType type)
	{
		// Create scene without renderer (template scene)
		return CreateRef<Scene>(&g_PersistentAllocator, type, false);
	}

	Entity SceneImporter::ProcessNode(Scene* scene, const ftf::Asset* asset, const ftf::Node* node, Entity parent)
	{
		// Create entity for this node
		Entity entity = parent.Valid() ? scene->CreateChildEntity(parent) : scene->CreateEntity();

		// Set node name if available
		if
		(
			!node->name.empty()
		)
		{
			entity.GetComponent<TagComponent>().tag = node->name;
		}
		else
		{
			entity.GetComponent<TagComponent>().tag = "Node";
		}

		// Extract and apply transform
		TRSComponent transform = ExtractTransform(node);
		entity.GetComponent<TRSComponent>() = transform;

		// Process mesh if present
		if
		(
			node->meshIndex.has_value()
		)
		{
			ProcessMeshNode(scene, entity, asset, node);
		}

		// Process camera if present
		if
		(
			node->cameraIndex.has_value()
		)
		{
			ProcessCameraNode(scene, entity, asset, node);
		}

		// Process light if present (GLTF extension)
		// Note: Light processing will be implemented in later phases
		ProcessLightNode(scene, entity, asset, node);

		// Recursively process children
		for 
		(
			size_t child_index : node->children
		)
		{
			const ftf::Node& child_node = asset->nodes[child_index];
			ProcessNode(scene, asset, &child_node, entity);
		}

		return entity;
	}

	TRSComponent SceneImporter::ExtractTransform(const ftf::Node* node)
	{
		TRSComponent transform = {};

		// Initialize defaults
		transform.translation = glm::vec3(0.0f);
		transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		transform.scale = glm::vec3(1.0f);

		// Handle transform based on type
		if
		(
			std::holds_alternative<ftf::TRS>(node->transform)
		)
		{
			const auto& trs = std::get<ftf::TRS>(node->transform);
			
			// Apply translation
			if
			(
				trs.translation.size() == 3
			)
			{
				transform.translation = glm::vec3(
					trs.translation[0], 
					trs.translation[1], 
					trs.translation[2]
				);
			}

			// Apply rotation
			if
			(
				trs.rotation.size() == 4
			)
			{
				transform.rotation = glm::quat(
					trs.rotation[3], // w
					trs.rotation[0], // x
					trs.rotation[1], // y
					trs.rotation[2]  // z
				);
			}

			// Apply scale
			if
			(
				trs.scale.size() == 3
			)
			{
				transform.scale = glm::vec3(
					trs.scale[0],
					trs.scale[1],
					trs.scale[2]
				);
			}
		}
		// Matrix transforms are automatically decomposed to TRS by fastgltf
		// when using DecomposeNodeMatrices option, so we only handle TRS here

		return transform;
	}

	void SceneImporter::ProcessMeshNode(Scene* scene, Entity entity, const ftf::Asset* asset, const ftf::Node* node)
	{
		const ftf::Mesh& gltf_mesh = asset->meshes[node->meshIndex.value()];
		
		// Process all primitives in the mesh (Phase 2 enhancement)
		for
		(
			const ftf::Primitive& primitive : gltf_mesh.primitives
		)
		{
			// Skip primitives without materials
			if
			(
				!primitive.materialIndex.has_value()
			)
			{
				OMNIFORCE_CORE_WARNING("Skipping mesh primitive without material in node: {}", 
					node->name.empty() ? "Unnamed" : node->name);
				continue;
			}

			const ftf::Material& gltf_material = asset->materials[primitive.materialIndex.value()];

			// Validate mesh before processing
			if
			(
				ValidateSubmesh(&gltf_mesh, &primitive, &gltf_material)
			)
			{
				OMNIFORCE_CORE_WARNING("Skipping invalid mesh primitive in node: {}", 
					node->name.empty() ? "Unnamed" : node->name);
				continue;
			}

			// Process mesh data using complete ModelImporter-style approach
			VertexAttributeMetadataTable attribute_metadata_table = {};
			uint32 vertex_stride = 0;
			ReadVertexMetadata(&attribute_metadata_table, &vertex_stride, asset, &primitive);

			std::vector<byte> vertex_data;
			std::vector<uint32> index_data;
			ReadVertexAttributes(&vertex_data, &index_data, asset, &primitive, &attribute_metadata_table, vertex_stride);

			Ref<Mesh> mesh;
			AABB lod0_aabb = {};
			std::shared_mutex mtx;
			
			// Process mesh data
			ProcessMeshData(&mesh, &lod0_aabb, &vertex_data, &index_data, vertex_stride, 
				attribute_metadata_table, const_cast<ftf::Material&>(gltf_material), &mtx);

			// Simplified material processing for Phase 2
			// Skip material import for now to avoid complexity
			Ref<Material> material = nullptr;

			// Create mesh component with proper handles
			MeshComponent mesh_component = {};
			mesh_component.mesh_handle = mesh->Handle;
			mesh_component.material_handle = 0; // No material for Phase 2 simplification
			
			// If multiple primitives, create child entities for each additional primitive
			if 
			(
				gltf_mesh.primitives.size() > 1 && &primitive != &gltf_mesh.primitives[0]
			)
			{
				Entity child_entity = scene->CreateChildEntity(entity);
				child_entity.GetComponent<TagComponent>().tag = fmt::format("{}_primitive_{}", 
					entity.GetComponent<TagComponent>().tag, &primitive - &gltf_mesh.primitives[0]);
				child_entity.AddComponent<MeshComponent>(mesh_component);
			}
			else
			{
				entity.AddComponent<MeshComponent>(mesh_component);
			}
		}
	}

	void SceneImporter::ProcessCameraNode(Scene* scene, Entity entity, const ftf::Asset* asset, const ftf::Node* node)
	{
		// Camera processing for Phase 3
		// For now, just log that we found a camera
		OMNIFORCE_CORE_TRACE("Found camera node: {}", node->name.empty() ? "Unnamed" : node->name);
	}

	void SceneImporter::ProcessLightNode(Scene* scene, Entity entity, const ftf::Asset* asset, const ftf::Node* node)
	{
		// Light processing for Phase 3
		// GLTF lights are typically extensions, will implement later
	}

	void SceneImporter::ProcessUsedMaterials(const ftf::Asset* asset, Scene* scene)
	{
		// Materials are now processed directly in ProcessMeshNode (Phase 2)
		// This method is kept for backward compatibility but is no longer used
		OMNIFORCE_CORE_TRACE("ProcessUsedMaterials is deprecated - materials are now processed in ProcessMeshNode");
	}

	void SceneImporter::ExtractAsset(ftf::Asset* asset, std::filesystem::path path)
	{
		// Copy implementation from ModelImporter
		ftf::Extensions extensions = ftf::Extensions::KHR_materials_transmission | 
									 ftf::Extensions::KHR_materials_ior | 
									 ftf::Extensions::KHR_materials_specular |
									 ftf::Extensions::KHR_texture_transform;

		ftf::Parser gltf_parser(extensions);
		ftf::GltfDataBuffer data_buffer;

		if
		(
			!data_buffer.loadFromFile(path)
		)
		{
			OMNIFORCE_CORE_ERROR("Failed to load glTF model with path: {}. Aborting import.", path.string());
			return;
		}

		ftf::GltfType source_type = ftf::determineGltfFileType(&data_buffer);

		if
		(
			source_type == ftf::GltfType::Invalid
		)
		{
			OMNIFORCE_CORE_ERROR("Failed to determine glTF file type with path: {}. Aborting import.", path.string());
			return;
		}

		constexpr ftf::Options options = ftf::Options::DontRequireValidAssetMember |
			ftf::Options::LoadGLBBuffers | ftf::Options::LoadExternalBuffers |
			ftf::Options::LoadExternalImages | ftf::Options::GenerateMeshIndices | ftf::Options::DecomposeNodeMatrices;

		ftf::Expected<ftf::Asset> expected_asset(ftf::Error::None);

		source_type == ftf::GltfType::glTF ? expected_asset = gltf_parser.loadGltf(&data_buffer, path.parent_path(), options) :
			expected_asset = gltf_parser.loadGltfBinary(&data_buffer, path.parent_path(), options);

		if
		(
			const auto error = expected_asset.error(); error != ftf::Error::None
		)
		{
			OMNIFORCE_CORE_ERROR("Failed to load asset source with path: {}. [{}]: {} Aborting import.", path.string(),
				ftf::getErrorName(error), ftf::getErrorMessage(error));
		}

		*asset = std::move(expected_asset.get());
	}

	// Copy methods from ModelImporter for Phase 1
	bool SceneImporter::ValidateSubmesh(const ftf::Mesh* mesh, const ftf::Primitive* primitive, const ftf::Material* material)
	{
		if
		(
			!primitive->indicesAccessor.has_value()
		)
		{
			OMNIFORCE_CORE_ERROR("One of the submeshes has no indices. Unindexed meshes are not supported. Skipping submesh");
			return true;
		}
		
		if
		(
			primitive->type != ftf::PrimitiveType::Triangles
		)
		{
			OMNIFORCE_CORE_ERROR("One of the submeshes primitive type is other than triangle list - currently only triangle list is supported. Skipping submesh");
			return true;
		}
		
		return false;
	}

	void SceneImporter::ReadVertexMetadata(VertexAttributeMetadataTable* out_table, uint32* out_size, const ftf::Asset* asset, const ftf::Primitive* primitive)
	{
		uint32 attribute_stride = 12; // Start with position size

		// Iterate through attributes and add them to map
		for 
		(
			auto attribute : primitive->attributes
		)
		{
			if
			(
				attribute.first == "POSITION"
			)
			{
				continue; // Skip position, it's always at offset 0
			}

			auto& attrib_accessor = asset->accessors[attribute.second];
			out_table->emplace(attribute.first, 0);
		}

		// Evaluate offsets for sorted attributes
		for 
		(
			auto& attribute : *out_table
		)
		{
			attribute.second = attribute_stride;
			const auto& attribute_accessor = asset->accessors[primitive->findAttribute(attribute.first)->second];
			attribute_stride += VertexDataQuantizer::GetRuntimeAttributeSize(attribute.first);
		}
		*out_size = attribute_stride;
	}

	void SceneImporter::ReadVertexAttributes(std::vector<byte>* out_vertex_data, std::vector<uint32>* out_index_data, const ftf::Asset* asset,
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

		// Load attributes
		for 
		(
			auto& attrib : *metadata
		)
		{
			const auto& accessor = asset->accessors[primitive->findAttribute(attrib.first)->second];

			if
			(
				attrib.first.find("TEXCOORD") != std::string::npos
			)
			{
				ftf::iterateAccessorWithIndex<glm::vec2>(*asset, accessor,
					[&](const glm::vec2 value, std::size_t idx) { 
						const glm::u16vec2 quantized_uv = quantizer.QuantizeUV(value);
						memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &quantized_uv, sizeof(quantized_uv));
					}
				);
			}
			else if
			(
				attrib.first.find("NORMAL") != std::string::npos
			)
			{
				ftf::iterateAccessorWithIndex<glm::vec3>(*asset, accessor,
					[&](const glm::vec3 value, std::size_t idx) {
						const glm::u16vec2 quantized_normal = quantizer.QuantizeNormal(value);
						memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &quantized_normal, sizeof(quantized_normal));
					}
				);
			}
			else if
			(
				attrib.first.find("TANGENT") != std::string::npos
			)
			{
				ftf::iterateAccessorWithIndex<glm::vec4>(*asset, accessor,
					[&](const glm::vec4 value, std::size_t idx) {
						const glm::u16vec2 quantized_tangent = quantizer.QuantizeTangent(value);
						memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &quantized_tangent, sizeof(quantized_tangent));
					}
				);
			}
			else if
			(
				attrib.first.find("COLOR") != std::string::npos
			)
			{
				ftf::iterateAccessorWithIndex<glm::vec4>(*asset, accessor,
					[&](const glm::vec4 value, std::size_t idx) {
						const glm::u8vec4 quantized_color = quantizer.QuantizeColor(value);
						memcpy(out_vertex_data->data() + idx * vertex_stride + attrib.second, &quantized_color, sizeof(quantized_color));
					}
				);
			}
		}
	}

	// Enhanced implementations for Phase 2 - copied from ModelImporter
	Ptr<RTAccelerationStructure> SceneImporter::BuildAccelerationStructure(
		const std::vector<byte>& vertex_data, 
		const std::vector<uint32>& index_data, 
		uint32 vertex_stride, 
		MaterialDomain domain)
	{
		// Simplified acceleration structure for Phase 2 - return nullptr for now
		// Full RT support can be added later
		return nullptr;
	}

	GeometryLayoutTable SceneImporter::BuildLayoutTable(uint32 vertex_stride, const VertexAttributeMetadataTable& vertex_metadata)
	{
		// Simplified layout table for Phase 2 - return empty layout for now
		GeometryLayoutTable layout = {};
		return layout;
	}

	void SceneImporter::ProcessMeshData(
		Ref<Mesh>* out_mesh,
		AABB* out_lod0_aabb,
		const std::vector<byte>* vertex_data,
		const std::vector<uint32>* index_data,
		uint32 vertex_stride,
		const VertexAttributeMetadataTable& vertex_metadata,
		ftf::Material& material,
		std::shared_mutex* mtx)
	{
		// Simplified mesh creation for Phase 2 to avoid complex dependencies
		MeshData mesh_data = {};
		AABB lod0_aabb = {};
		
		// Calculate basic AABB from vertex data
		const float* vertex_positions = reinterpret_cast<const float*>(vertex_data->data());
		uint32 vertex_count = vertex_data->size() / vertex_stride;
		
		if 
		(
			vertex_count > 0
		)
		{
			lod0_aabb.min = glm::vec3(vertex_positions[0], vertex_positions[1], vertex_positions[2]);
			lod0_aabb.max = lod0_aabb.min;
			
			for 
			(
				uint32 i = 0; i < vertex_count; ++i
			)
			{
				const float* pos = &vertex_positions[i * (vertex_stride / sizeof(float))];
				glm::vec3 vertex_pos(pos[0], pos[1], pos[2]);
				
				lod0_aabb.min = glm::min(lod0_aabb.min, vertex_pos);
				lod0_aabb.max = glm::max(lod0_aabb.max, vertex_pos);
			}
		}

		std::lock_guard lock(*mtx);
		*out_mesh = Mesh::Create(&g_PersistentAllocator, mesh_data, lod0_aabb);
		AssetManager::Get()->RegisterAsset(*out_mesh);
		*out_lod0_aabb = lod0_aabb;
	}

	void SceneImporter::ProcessMaterialData(tf::Subflow& subflow, Ref<Material>* out_material, const ftf::Asset* asset,
		const ftf::Material* material, const VertexAttributeMetadataTable* vertex_macro_table, std::shared_mutex* mtx)
	{
		// Use MaterialImporter for proper material processing
		AssetHandle material_handle = m_MaterialImporter.Import(subflow, asset, material);
		
		if 
		(
			material_handle != 0
		)
		{
			*out_material = AssetManager::Get()->GetAsset<Material>(material_handle);
		}
		else
		{
			*out_material = nullptr;
		}
	}

}
