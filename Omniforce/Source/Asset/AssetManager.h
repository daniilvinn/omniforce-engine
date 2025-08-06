#pragma once

#include <Foundation/Common.h>
#include <RHI/Image.h>

#include <filesystem>
#include <shared_mutex>

#include <robin_hood.h>

namespace Omni {

	class OMNIFORCE_API AssetManager {
	public:
		AssetManager();
		~AssetManager();

		static void Init();
		static void Shutdown();
		void FullUnload();

		static AssetManager* Get() { return s_Instance; }

		AssetHandle LoadAssetSource(std::filesystem::path path, const AssetHandle& id = AssetHandle());
		AssetHandle RegisterAsset(Ref<AssetBase> asset, const AssetHandle& id = AssetHandle());
		bool HasAsset(AssetHandle id) { return m_AssetRegistry.contains(id); }

		template<typename ResourceType> // where ResourceType is child of AssetType: Image, Mesh, Material etc.
		Ref<ResourceType> GetAsset(AssetHandle id) { return m_AssetRegistry.at(id); }
		AssetHandle GetHandle(std::filesystem::path path) const { return m_UUIDs.at(path.string()); }
		auto* GetAssetRegistry() const { return &m_AssetRegistry; }

	private:
		AssetHandle ImportImage(std::filesystem::path path, AssetHandle handle);
		AssetHandle ImportMeshSource(std::filesystem::path path, AssetHandle handle);
		AssetHandle ImportImageSource(std::filesystem::path path, AssetHandle handle);

	private:
		inline static AssetManager* s_Instance;
		robin_hood::unordered_map<AssetHandle, Ref<AssetBase>> m_AssetRegistry;
		robin_hood::unordered_map<std::filesystem::path, UUID> m_UUIDs;
		std::shared_mutex m_Mutex;
	};

}	