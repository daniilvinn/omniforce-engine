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

#include <set>

#include <glm/gtc/type_precision.hpp>
#include <glm/glm.hpp>
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

		// Process used materials after all nodes are processed
		ProcessUsedMaterials(&ftf_asset, template_scene.Raw());

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
		// For other transform types (e.g., matrix), use default transform for Phase 1
		// This will be properly implemented in later phases

		return transform;
	}

	void SceneImporter::ProcessMeshNode(Scene* scene, Entity entity, const ftf::Asset* asset, const ftf::Node* node)
	{
		const ftf::Mesh& gltf_mesh = asset->meshes[node->meshIndex.value()];
		
		// For Phase 1, we'll process only the first primitive
		// Later phases will handle multiple primitives per mesh
		if
		(
			!gltf_mesh.primitives.empty()
		)
		{
			const ftf::Primitive& primitive = gltf_mesh.primitives[0];
			const ftf::Material& gltf_material = asset->materials[primitive.materialIndex.value()];

			// Validate mesh before processing
			if
			(
				ValidateSubmesh(&gltf_mesh, &primitive, &gltf_material)
			)
			{
				OMNIFORCE_CORE_WARNING("Skipping invalid mesh primitive in node: {}", 
					node->name.empty() ? "Unnamed" : node->name);
				return;
			}

			// Process mesh data (simplified for Phase 1)
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

			// Create material (simplified for Phase 1)
			Ref<Material> material;
			// TODO: Proper material processing in later phases

			// For now, create a placeholder mesh component
			// The material handle will be set during ProcessUsedMaterials
			MeshComponent mesh_component = {};
			mesh_component.mesh_handle = mesh->Handle;
			mesh_component.material_handle = 0; // Will be set by ProcessUsedMaterials
			
			entity.AddComponent<MeshComponent>(mesh_component);
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
		// Collect all material indices that are actually used by meshes in the scene
		std::set<uint32> used_material_indices;
		
		auto mesh_view = scene->GetRegistry()->view<MeshComponent>();
		for 
		(
			auto entity_id : mesh_view
		)
		{
			// For Phase 1, we need to find which material index this mesh used
			// This is a simplified approach - in later phases we'll track this properly
		}

		// Process only the used materials
		// For Phase 1, this is simplified and will be enhanced later
		OMNIFORCE_CORE_TRACE("Material filtering will be implemented in later Phase 1 iterations");
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

	// Simplified stub methods for Phase 1 - full implementations in later phases
	Ptr<RTAccelerationStructure> SceneImporter::BuildAccelerationStructure(
		const std::vector<byte>& vertex_data, 
		const std::vector<uint32>& index_data, 
		uint32 vertex_stride, 
		MaterialDomain domain)
	{
		// Simplified implementation - will be properly implemented
		return nullptr;
	}

	GeometryLayoutTable SceneImporter::BuildLayoutTable(uint32 vertex_stride, const VertexAttributeMetadataTable& vertex_metadata)
	{
		// Simplified implementation - will be properly implemented
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
		// Simplified mesh creation for Phase 1
		MeshData mesh_data = {};
		AABB lod0_aabb = {};
		
		*out_mesh = Mesh::Create(&g_PersistentAllocator, mesh_data, lod0_aabb);
		AssetManager::Get()->RegisterAsset(*out_mesh);
	}

	void SceneImporter::ProcessMaterialData(tf::Subflow& subflow, Ref<Material>* out_material, const ftf::Asset* asset,
		const ftf::Material* material, const VertexAttributeMetadataTable* vertex_macro_table, std::shared_mutex* mtx)
	{
		// Simplified material creation for Phase 1
		// Will use MaterialImporter in later phases
	}

}
