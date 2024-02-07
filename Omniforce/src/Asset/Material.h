#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"
#include "Asset/AssetBase.h"

#include <variant>
#include <map>

namespace Omni {

	using MaterialProperty = std::variant<AssetHandle, float32, uint32, glm::vec4>;

	class OMNIFORCE_API Material : public AssetBase {
	public:
		Material(std::string name, AssetHandle id = AssetHandle()) {
			m_Name = std::move(name);
			Type = AssetType::MATERIAL;
			Handle = id;
		}

		~Material() {}
		static Shared<Material> Create(std::string name, AssetHandle id = AssetHandle());
		static uint8 GetRuntimeEntrySize(uint8 variant_index);

		void Destroy() override;

		const auto& GetName() const { return m_Name; }
		const auto& GetTable() const { return m_Properties; }
		
		template<typename T>
		void AddProperty(std::string_view key, T property) { m_Properties.emplace(key, property); }
		void RemoveResource(std::string_view key) { m_Properties.erase(key.data()); }

		void AddShaderMacro(const std::string& macro, const std::string& value = "") {
			m_Macros.push_back({ macro, value });
		}

	private:
		std::string m_Name;
		std::map<std::string, MaterialProperty> m_Properties;
		std::vector<std::pair<std::string, std::string>> m_Macros;


	};

}