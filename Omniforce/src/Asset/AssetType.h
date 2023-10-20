#pragma once

#include "Foundation/Types.h"

namespace Omni {

	enum class AssetType : uint8 {
		MESH_SRC,
		MATERIAL,
		AUDIO_SRC,
		IMAGE, // either texture or even render target
		UNKNOWN
	};

	inline static const robin_hood::unordered_map<std::string, AssetType> g_AssetTypesExtensionMap = {
		{".png", AssetType::IMAGE},
		{".jpg", AssetType::IMAGE},
		{".jpeg", AssetType::IMAGE},

		{".fbx", AssetType::MESH_SRC},
		{".gltf", AssetType::MESH_SRC},
		{".obj", AssetType::MESH_SRC},

		{".mp3", AssetType::AUDIO_SRC},
		{".wav", AssetType::AUDIO_SRC},
		
	};

	inline AssetType FileExtensionToAssetType(std::string_view extension) {
		if (g_AssetTypesExtensionMap.contains(extension.data()))
			return g_AssetTypesExtensionMap.at(extension.data());

		return AssetType::UNKNOWN;
	}

}