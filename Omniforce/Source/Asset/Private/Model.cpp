#include <Foundation/Common.h>
#include <Asset/Model.h>

namespace Omni {

	Model::Model(const std::vector<std::pair<AssetHandle, AssetHandle>>& meshes)
		: m_Meshes(meshes)
	{
		
	}

	Shared<Model> Model::Create(const std::vector<std::pair<AssetHandle, AssetHandle>>& meshes)
	{
		return std::make_shared<Model>(meshes);
	}

}