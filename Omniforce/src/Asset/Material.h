#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"
#include "Asset/AssetBase.h"

#include <robin_hood.h>

namespace Omni {

	class OMNIFORCE_API Material : public AssetBase {
	public:
		Material(std::string name, AssetHandle id = AssetHandle());
		~Material();

		void Destroy() override;

		const auto& GetName() const { return m_Name; }
		const auto& GetTable() const { return m_Images; }
		
		void AddResource(std::string_view key, AssetHandle resource) { m_Images.emplace(key, resource); }
		void RemoveResource(std::string_view key) { m_Images.erase(key.data()); }

	private:
		std::string m_Name;
		robin_hood::unordered_map<std::string, AssetHandle> m_Images;
		UUID m_Id;

	};

}