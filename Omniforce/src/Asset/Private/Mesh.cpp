#include "../Mesh.h"
#include "../AssetManager.h"
#include "Log/Logger.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Omni {

	Shared<Mesh> Mesh::Create(std::filesystem::path path, AssetHandle handle)
	{
		return std::make_shared<Mesh>(path, handle);
	}

	Mesh::Mesh(std::filesystem::path path, AssetHandle handle)
	{
		Type = AssetType::MESH_SRC;
		Handle = handle;

		Assimp::Importer imp;
		AssetManager* asset_manager = AssetManager::Get();

		const aiScene* scene = imp.ReadFile(path.string(),
			aiProcessPreset_TargetRealtime_Fast |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_FlipUVs
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			OMNIFORCE_CORE_ERROR("Assimp: {0}", imp.GetErrorString());
			return;
		}

		for (int32 i = 0; i < scene->mNumMaterials; i++) {
			aiMaterial* mat = scene->mMaterials[i];

			aiString albedo_filepath;
			mat->GetTexture(aiTextureType_BASE_COLOR, 0, &albedo_filepath);

			if (albedo_filepath.length)
				asset_manager->LoadAssetSource(albedo_filepath.C_Str());
		}

		m_Valid = true;
	}

	Mesh::~Mesh()
	{

	}

}
