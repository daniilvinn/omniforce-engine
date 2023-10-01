#include "../Mesh.h"
#include "Log/Logger.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Omni {

	Mesh::Mesh(std::filesystem::path path)
	{
		Assimp::Importer imp;

		const aiScene* scene = imp.ReadFile(path.string(), 
			aiProcessPreset_TargetRealtime_Fast |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_FlipUVs
		);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			OMNIFORCE_CORE_ERROR("Assimp: {0}", imp.GetErrorString());

		m_Valid = true;
	}

}
