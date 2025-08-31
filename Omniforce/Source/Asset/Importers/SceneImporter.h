#pragma once

#include <Foundation/Common.h>
#include <Asset/Importers/MaterialImporter.h>
#include <Asset/Material.h>
#include <Rendering/Mesh.h>
#include <Scene/Scene.h>

#include <filesystem>
#include <shared_mutex>
#include <map>

#include <taskflow/taskflow.hpp>

namespace fastgltf {
	class Asset;
	class Node;
	class Mesh;
	class Material;
	class Primitive;
}

namespace Omni {

	namespace ftf = fastgltf;
	
	class Scene;
	class Entity;

	using MeshMaterialPair = std::pair<AssetHandle, AssetHandle>;
	using VertexAttributeMetadataTable = std::map<std::string, uint8>;

	class OMNIFORCE_API SceneImporter {
	public:
		// Import GLTF scene and return a new Scene object WITHOUT renderer
		// The returned scene is a "template" that can be merged into active scenes
		Ref<Scene> ImportScene(std::filesystem::path path, SceneType type = SceneType::SCENE_TYPE_3D);
		
	private:
		/*
		*  Extract and validate fastgltf::Asset
		*/
		void ExtractAsset(ftf::Asset* asset, std::filesystem::path path);

		/*
		*  Create template scene without renderer
		*/
		Ref<Scene> CreateTemplateScene(SceneType type);

		/*
		*  Process GLTF nodes recursively
		*/
		Entity ProcessNode(Scene* scene, const ftf::Asset* asset, const ftf::Node* node, Entity parent);
		
		/*
		*  Specialized node processors
		*/
		void ProcessMeshNode(Scene* scene, Entity entity, const ftf::Asset* asset, const ftf::Node* node);
		void ProcessCameraNode(Scene* scene, Entity entity, const ftf::Asset* asset, const ftf::Node* node);  
		void ProcessLightNode(Scene* scene, Entity entity, const ftf::Asset* asset, const ftf::Node* node);
		
		/*
		*  Transform utilities
		*/
		TRSComponent ExtractTransform(const ftf::Node* node);
		
		/*
		*  Asset processing - only import materials that are actually used by meshes
		*/
		void ProcessUsedMaterials(const ftf::Asset* asset, Scene* scene);

		/*
		*  Used to validate support of the mesh and use returned result further for conditional tasking
		*/
		bool ValidateSubmesh(const ftf::Mesh* mesh, const ftf::Primitive* primitive, const ftf::Material* material);

		/*
		*  Evaluate attribute offsets and vertex stride
		*/
		void ReadVertexMetadata(VertexAttributeMetadataTable* out_table, uint32* out_size, const ftf::Asset* asset, const ftf::Primitive* mesh);

		/*
		*  Build acceleration structure for ray tracing
		*/
		Ptr<RTAccelerationStructure> BuildAccelerationStructure(
			const std::vector<byte>& vertex_data,
			const std::vector<uint32>& index_data,
			uint32 vertex_stride,
			MaterialDomain domain
		);

		/*
		*  Builds device struct of attribute layout
		*/
		GeometryLayoutTable BuildLayoutTable(uint32 vertex_stride, const VertexAttributeMetadataTable& vertex_metadata);

		/*
		*  Read vertex and index data to buffers
		*/
		void ReadVertexAttributes(std::vector<byte>* out_vertex_data, std::vector<uint32>* out_index_data, const ftf::Asset* asset,
			const ftf::Primitive* mesh, const VertexAttributeMetadataTable* metadata, uint32 vertex_stride);

		/*
		*  Process vertex data: optimize, generate lods, meshlets and create Mesh objects
		*/
		void ProcessMeshData(
			Ref<Mesh>* out_mesh,
			AABB* out_lod0_aabb,
			const std::vector<byte>* vertex_data,
			const std::vector<uint32>* index_data,
			uint32 vertex_stride,
			const VertexAttributeMetadataTable& vertex_metadata,
			ftf::Material& material,
			std::shared_mutex* mtx
		);

		/*
		*  Process material data. Load image, generate mip-maps, compress data and create Material objects
		*/
		void ProcessMaterialData(tf::Subflow& properties_load_subflow, Ref<Material>* out_material, const ftf::Asset* asset,
			const ftf::Material* material, const VertexAttributeMetadataTable* vertex_macro_table, std::shared_mutex* mtx);

	private:
		// Reuse existing material/mesh processing from ModelImporter
		MaterialImporter m_MaterialImporter;
	};

}
