#pragma once

#include <Foundation/Common.h>
#include <Asset/AssetBase.h>
#include <Renderer/Pipeline.h>

#include <variant>
#include <map>

namespace Omni {

	using MaterialTextureProperty = std::pair<AssetHandle, uint32>; // texture id - uv channel
	using MaterialProperty = std::variant<MaterialTextureProperty, float32, uint32, glm::vec4>;

	enum class MaterialDomain {
		OPAQUE,
		MASKED,
		TRANSLUCENT,
		NONE
	};

	class OMNIFORCE_API Material : public AssetBase {
	public:
		Material(std::string name, AssetHandle id = AssetHandle()) {
			m_Name = std::move(name);
			Type = AssetType::MATERIAL;
			Handle = id;
		}

		~Material() {}
		static Ref<Material> Create(IAllocator* allocator, std::string name, AssetHandle id = AssetHandle());
		void Destroy() override;

		template<typename T>
		void AddProperty(std::string_view key, T property) { m_Properties.emplace(key, property); }
		void RemoveResource(std::string_view key) { m_Properties.erase(key.data()); }

		void AddShaderMacro(const std::string& macro, const std::string& value = "") {
			m_Macros.emplace(macro, value);
		}

		static uint8 GetRuntimeEntrySize(uint8 variant_index);
		const auto& GetName() const { return m_Name; }
		const auto& GetTable() const { return m_Properties; }
		const Ref<Pipeline> GetPipeline() const { return m_Pipeline; }

		void CompilePipeline();

		void SetDomain(MaterialDomain domain) { m_Domain = domain; }
		MaterialDomain GetDomain() const { return m_Domain; }
	private:
		std::string m_Name;
		std::map<std::string, MaterialProperty> m_Properties;
		ShaderMacroTable m_Macros;
		Ref<Pipeline> m_Pipeline;
		MaterialDomain m_Domain;
	};

}