#include <Foundation/Common.h>
#include <Asset/Model.h>

namespace Omni {

	Model::Model(const std::vector<std::pair<AssetHandle, AssetHandle>>& meshes)
		: m_Meshes(meshes)
	{
		
	}

	Ref<Model> Model::Create(IAllocator* allocator, std::vector<std::pair<AssetHandle, AssetHandle>>& meshes)
	{
		return CreateRef<Model>(allocator, meshes);
	}

}