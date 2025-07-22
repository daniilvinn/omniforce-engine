#pragma once

#include <Foundation/Common.h>
#include <Asset/Material.h>
#include <Rendering/Mesh.h>

#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <map> 

#include <taskflow/taskflow.hpp>

namespace fastgltf {
	class Asset;
	class Mesh;
	class Material;
	class Primitive;
}

namespace Omni {

	namespace ftf = fastgltf;

	using MeshMaterialPair = std::pair<AssetHandle, AssetHandle>;
	using VertexAttributeMetadataTable = std::map<std::string, uint8>;

	class OMNIFORCE_API ModelImporter {
	public:
		AssetHandle Import(std::filesystem::path path);

	private:
		/*
		*  Extract and validate fastgltf::Asset
		*/
		void ExtractAsset(ftf::Asset* asset, std::filesystem::path path);

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
			const ftf::Primitive* mesh, const VertexAttributeMetadataTable* metadata, uint32 vertex_stride );

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
	};

}