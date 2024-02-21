#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"
#include "Asset/AssetBase.h"
#include "Renderer/Pipeline.h"

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
		void Destroy() override;

		template<typename T>
		void AddProperty(std::string_view key, T property) { m_Properties.emplace(key, property); }
		void RemoveResource(std::string_view key) { m_Properties.erase(key.data()); }

		void AddShaderMacro(const std::string& macro, const std::string& value = "") {
			m_Macros.push_back(std::make_pair(macro, value));
		}

		static uint8 GetRuntimeEntrySize(uint8 variant_index);
		const auto& GetName() const { return m_Name; }
		const auto& GetTable() const { return m_Properties; }
		const Shared<Pipeline> GetPipeline() const { return m_Pipeline; }

		void CompilePipeline();
		
	private:
		std::string m_Name;
		std::map<std::string, MaterialProperty> m_Properties;
		ShaderMacroTable m_Macros;
		Shared<Pipeline> m_Pipeline;

	};

}