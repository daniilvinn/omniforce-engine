#pragma once

#include <Foundation/Common.h>
#include <Asset/AssetBase.h>

namespace Omni {

	// Collection of Mesh-Material pairs.
	// This class is used just for convenience of handling meshes with multiple submeshes.
	// This is NOT prefab - all submeshes have no transforms, hence there are no hierarchy of nodes in this class.
	class OMNIFORCE_API Model : public AssetBase {
	public:
		Model(const std::vector<std::pair<AssetHandle, AssetHandle>>& meshes);
		~Model() {};

		static Shared<Model> Create(const std::vector<std::pair<AssetHandle, AssetHandle>>& meshes);

		void Destroy() override {};
		
		auto& GetMap() { return m_Meshes; }

	private:
		std::vector<std::pair<AssetHandle, AssetHandle>> m_Meshes; // Mesh - material pairs
	};

}