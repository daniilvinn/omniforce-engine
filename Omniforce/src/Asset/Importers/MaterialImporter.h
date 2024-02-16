#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Asset/Material.h>

#include <fastgltf/types.hpp>
#include <fmt/format.h>

#include <shared_mutex>

namespace Omni {

	namespace ftf = fastgltf;

	// Internal for engine. To be used only in other importers
	class MaterialImporter {
	public:
		AssetHandle Import(const ftf::Asset& root, const ftf::Material& in_material);

	private:
		template<typename T> void HandleProperty(std::string_view key, const T& property, Shared<Material> material, const ftf::Asset & root, std::shared_mutex& mutex)
		{
			mutex.lock();
			material->AddProperty(key, property);
			material->AddShaderMacro(fmt::format("__OMNI_HAS_{}", key));
			mutex.unlock();
		}

		AssetHandle LoadTextureProperty(uint64 texture_index, const ftf::Asset& root);

	};

}