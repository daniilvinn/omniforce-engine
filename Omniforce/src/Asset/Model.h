#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Asset/AssetBase.h>

namespace Omni {

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