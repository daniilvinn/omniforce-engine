#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"
#include <Renderer/Image.h>
#include <Memory/Pointers.hpp>

#include <filesystem>
#include <shared_mutex>

#include "robin_hood.h"

namespace Omni {

	class OMNIFORCE_API AssetManager {
	public:
		AssetManager();
		~AssetManager();

		static void Init();
		static void Shutdown();
		void FullUnload();

		static AssetManager* Get() { return s_Instance; }

		/*
		*  @brief loads asset source (.jpg, .gltf etc.) and creates Omniforce resource file in current project's directory.
		*  @return handle to an asset in asset registry.
		*/
		AssetHandle LoadAssetSource(std::filesystem::path path, const AssetHandle& id = AssetHandle());
		AssetHandle RegisterAsset(Shared<AssetBase> asset, const AssetHandle& id = AssetHandle());
		bool HasAsset(AssetHandle id) { m_AssetRegistry.contains(id); }

		template<typename ResourceType> // where ResourceType is child of AssetType: Image, Mesh, Material etc.
		Shared<ResourceType> GetAsset(AssetHandle id) { return ShareAs<ResourceType>(m_AssetRegistry.at(id)); }
		AssetHandle GetHandle(std::filesystem::path path) const { return m_UUIDs.at(path.string()); }
		auto* GetAssetRegistry() const { return &m_AssetRegistry; }

	private:
		AssetHandle ImportImage(std::filesystem::path path, AssetHandle handle);
		AssetHandle ImportMeshSource(std::filesystem::path path, AssetHandle handle);
		AssetHandle ImportImageSource(std::filesystem::path path, AssetHandle handle);

	private:
		inline static AssetManager* s_Instance;
		robin_hood::unordered_map<AssetHandle, Shared<AssetBase>> m_AssetRegistry;
		robin_hood::unordered_map<std::filesystem::path, UUID> m_UUIDs;
		std::shared_mutex m_Mutex;
	};

}	