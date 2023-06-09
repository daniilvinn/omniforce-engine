#include "../AssetManager.h"

namespace Omni {

	AssetManager::AssetManager()
	{

	}

	AssetManager::~AssetManager()
	{
		for (auto& [id, texture] : m_TextureRegistry)
			texture->Destroy();
	}

	void AssetManager::Init()
	{
		s_Instance = new AssetManager;
	}

	void AssetManager::Shutdown()
	{
		delete s_Instance;
	}

	UUID AssetManager::LoadTexture(std::filesystem::path path, const UUID& id)
	{
		if (m_UUIDs.contains(path.string()))
			return m_UUIDs.at(path.string());

		ImageSpecification texture_spec = {};
		texture_spec.path = path;

		Shared<Image> image = Image::Create(texture_spec, id);

		m_TextureRegistry.emplace(image->GetId(), image);
		m_UUIDs.emplace(path.string(), image->GetId());

		return image->GetId();
	}

}