#pragma once

#include "Foundation/Types.h"

namespace Omni {

	enum class AssetType : uint8 {
		MESH_SRC,
		MATERIAL,
		AUDIO_SRC,
		IMAGE_SRC,
		OMNI_IMAGE,
		OMNI_MESH,
		UNKNOWN
	};

	inline static const robin_hood::unordered_map<std::string, AssetType> g_AssetTypesExtensionMap = {
		{".png", AssetType::IMAGE_SRC},
		{".jpg", AssetType::IMAGE_SRC},
		{".jpeg", AssetType::IMAGE_SRC},

		{".fbx", AssetType::MESH_SRC},
		{".gltf", AssetType::MESH_SRC},
		{".obj", AssetType::MESH_SRC},

		{".mp3", AssetType::AUDIO_SRC},
		{".wav", AssetType::AUDIO_SRC},
		{".oft", AssetType::OMNI_IMAGE},
		{".ofm", AssetType::OMNI_MESH},
	};

	inline AssetType FileExtensionToAssetType(std::string_view extension) {
		if (g_AssetTypesExtensionMap.contains(extension.data()))
			return g_AssetTypesExtensionMap.at(extension.data());

		return AssetType::UNKNOWN;
	}

}