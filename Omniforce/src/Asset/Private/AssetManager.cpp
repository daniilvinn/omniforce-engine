#include "../AssetManager.h"
#include "../AssetCompressor.h"
#include "Core/Utils.h"
#include "Filesystem/Filesystem.h"


#include <stb_image.h>

namespace Omni {

	AssetManager::AssetManager()
	{

	}

	AssetManager::~AssetManager()
	{
		FullUnload();
	}

	void AssetManager::Init()
	{
		stbi_set_flip_vertically_on_load(true);
		s_Instance = new AssetManager;
	}

	void AssetManager::Shutdown()
	{
		delete s_Instance;
	}

	void AssetManager::FullUnload()
	{
		for (auto& [id, asset] : m_AssetRegistry)
			asset->Destroy();
		m_AssetRegistry.clear();
		m_UUIDs.clear();
	}

	AssetHandle AssetManager::LoadAssetSource(std::filesystem::path path, const AssetHandle& id /*= AssetHandle()*/)
	{
		AssetType asset_type = FileExtensionToAssetType(path.extension().string());
		
		switch (asset_type)
		{
		case AssetType::MESH_SRC:			return ImportMeshSource(path, id);
		case AssetType::MATERIAL:			break;
		case AssetType::AUDIO_SRC:			break;
		case AssetType::IMAGE:				return ImportTexture(path, id);
		case AssetType::UNKNOWN:			return AssetHandle(0);
		}

	}

	AssetHandle AssetManager::ImportMeshSource(std::filesystem::path path, AssetHandle handle)
	{
		if (m_UUIDs.contains(path.string())) 
			return m_UUIDs.at(path.string());

		Shared<Mesh> mesh = Mesh::Create(path, handle);

		m_AssetRegistry.emplace(mesh->Handle, mesh);
		m_UUIDs.emplace(path.string(), mesh->Handle);

		return mesh->Handle;
	}

	AssetHandle AssetManager::ImportTexture(std::filesystem::path path, AssetHandle handle)
	{
		if (m_UUIDs.contains(path.string()))
			return m_UUIDs.at(path.string());

		int32 image_width, image_height, channels;
		RGBA32* raw_image_data = (RGBA32*)stbi_load(path.string().c_str(), &image_width, &image_height, &channels, STBI_rgb_alpha);
		
		std::vector<RGBA32> image_data;
		image_data.assign(raw_image_data, raw_image_data + (image_width * image_height));

		// Generate mip map
		std::vector<RGBA32> full_image_data = AssetCompressor::GenerateMipMaps(image_data, image_width, image_height);

		std::vector<byte> raw(full_image_data.size() * sizeof RGBA32);
		memcpy(raw.data(), full_image_data.data(), full_image_data.size() * sizeof RGBA32);
		full_image_data.clear();

		ImageSpecification texture_spec = {};
		texture_spec.pixels = std::move(raw);
		texture_spec.format = ImageFormat::RGBA32_UNORM;
		texture_spec.type = ImageType::TYPE_2D;
		texture_spec.usage = ImageUsage::TEXTURE;
		texture_spec.extent = { (uint32)image_width, (uint32)image_height, 1 };
		texture_spec.array_layers = 1;
		texture_spec.mip_levels = Utils::ComputeNumMipLevelsBC7(image_width, image_height) + 1;
		texture_spec.path = path;

		Shared<Image> image = Image::Create(texture_spec, handle);

		m_AssetRegistry.emplace(image->Handle, image);
		m_UUIDs.emplace(path.string(), image->Handle);

		return image->Handle;
	}

}