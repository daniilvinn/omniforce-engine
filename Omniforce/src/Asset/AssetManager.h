#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"
#include "Mesh.h"
#include <Renderer/Image.h>
#include <Memory/Pointers.hpp>

#include <filesystem>

#include "robin_hood.h"

namespace Omni {

	using AssetHandle = UUID;

	class OMNIFORCE_API AssetManager {
	public:
		AssetManager();
		~AssetManager();

		static void Init();
		static void Shutdown();
		void FullUnload();

		static AssetManager* Get() { return s_Instance; }

		AssetHandle LoadTexture(std::filesystem::path path, const UUID& id = UUID());
		AssetHandle LoadMesh(std::filesystem::path path, const UUID& id = UUID());
		bool HasTexture(std::filesystem::path path) { m_UUIDs.contains(path.string()); }
		bool HasMesh(std::filesystem::path path) { m_UUIDs.contains(path.string()); };

		Shared<Image> GetTexture(const AssetHandle& id) const { return m_TextureRegistry.at(id); }
		AssetHandle GetHandle(std::filesystem::path path) const { return m_UUIDs.at(path.string()); }
		auto* GetTextureRegistry() const { return &m_TextureRegistry; }

	private:
		inline static AssetManager* s_Instance;
		robin_hood::unordered_map<UUID, Shared<Image>> m_TextureRegistry;
		robin_hood::unordered_map<UUID, Shared<Mesh>> m_MeshRegistry;
		robin_hood::unordered_map<std::string, UUID> m_UUIDs;
	};

}	