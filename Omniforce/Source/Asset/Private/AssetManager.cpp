#include "../AssetManager.h"
#include "../AssetCompressor.h"
#include "Core/Utils.h"
#include "Filesystem/Filesystem.h"
#include "Threading/JobSystem.h"
#include "Log/Logger.h"

#include <fstream>

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
		OMNIFORCE_CORE_TRACE("Initialized asset manager");
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
		case AssetType::IMAGE_SRC:			return ImportImageSource(path, id);
		case AssetType::UNKNOWN:			return AssetHandle(0);
		default:							return AssetHandle(0);
		}

		return 0;
	}

	AssetHandle AssetManager::RegisterAsset(Shared<AssetBase> asset, const AssetHandle& id /*= AssetHandle()*/)
	{
		asset->Handle = id;
		m_Mutex.lock();
		m_AssetRegistry.emplace(id, asset);
		m_Mutex.unlock();

		return id;
	}

	AssetHandle AssetManager::ImportMeshSource(std::filesystem::path path, AssetHandle handle)
	{
		OMNIFORCE_ASSERT_TAGGED(false, "Not implemented");

		return 0;
	}

	AssetHandle AssetManager::ImportImageSource(std::filesystem::path path, AssetHandle handle)
	{
		if (m_UUIDs.contains(path.string()))
			return m_UUIDs.at(path.string());


		uint32 image_width, image_height;
		int32 channels;
		RGBA32* raw_image_data = (RGBA32*)stbi_load(path.string().c_str(), (int32*)&image_width, (int32*)&image_height, &channels, STBI_rgb_alpha);
		uint32 num_mip_levels = Utils::ComputeNumMipLevelsBC7(image_width, image_height) + 1;
		
		std::vector<RGBA32> image_data;
		image_data.assign(raw_image_data, raw_image_data + (image_width * image_height));

		// Generate mip map
		std::vector<RGBA32> image_data_with_mips = AssetCompressor::GenerateMipMaps(image_data, image_width, image_height);
		std::vector<byte> bc7_encoded_data = AssetCompressor::CompressBC7(image_data_with_mips, image_width, image_height, num_mip_levels);

		// Compress by GDeflate and write to a file
		auto executor = JobSystem::GetExecutor();

		tf::Taskflow taskflow;
		taskflow.emplace([bc7_encoded_data, path, num_mip_levels, image_width, image_height]() mutable {
			// Open file stream
			std::filesystem::path final_path = FileSystem::GetWorkingDirectory() / "assets/compressed" / (path.stem().string() + ".oft");
			std::ofstream fout(final_path, std::ios::binary | std::ios::out | std::ios::beg);
			
			std::stringstream subresource_data_stream;

			// Configure file header
			AssetFileHeader file_header = {};
			file_header.header_size = sizeof(AssetFileHeader);
			file_header.asset_type = AssetType::IMAGE_SRC;
			file_header.uncompressed_data_size = bc7_encoded_data.size();
			file_header.subresources_size = 0;
			file_header.additional_data = (uint64)image_width | (uint64)image_height << 32;

			// Compute metadata for subresources
			std::array<AssetFileSubresourceMetadata, 16> subresources_metadata = {};

			uint32 current_subresource_offset = 0;
			for (uint32 i = 0; i < num_mip_levels; i++) {
				uint32 current_mip_size = (image_width >> i) * (image_height >> i);
				subresources_metadata[i].offset = current_subresource_offset;
				subresources_metadata[i].compressed = current_mip_size >= AssetCompressor::GDEFLATE_PAGE_SIZE / 2;
				subresources_metadata[i].decompressed_size = current_mip_size;
				subresources_metadata[i].size = 0; // TODO
				
				auto subresource_source_data_begin = bc7_encoded_data.begin() + current_subresource_offset;
				auto subresource_source_data_end = subresource_source_data_begin + current_mip_size;

				if (subresources_metadata[i].compressed) {
					subresources_metadata[i].size = AssetCompressor::CompressGDeflate(
						{ subresource_source_data_begin, subresource_source_data_end },
						&subresource_data_stream
					);
					std::vector<char> test(subresource_data_stream.tellp());
					subresource_data_stream.read(test.data(), subresource_data_stream.tellp());

					file_header.subresources_size += subresources_metadata[i].size;
					current_subresource_offset += subresources_metadata[i].size;
				}
				else {
					subresources_metadata[i].size = current_mip_size;
					current_subresource_offset += subresources_metadata[i].size;
					file_header.subresources_size += current_mip_size;
					subresource_data_stream.write((const char*)&(*subresource_source_data_begin), subresource_source_data_end - subresource_source_data_begin);
				}
			}
			
			// Write header and subresource metadata
			fout.write((const char*)&file_header, sizeof(file_header));
			fout.write((const char*)subresources_metadata.data(), sizeof(AssetFileSubresourceMetadata) * subresources_metadata.size());
			
			std::vector<char> subresources_data(subresource_data_stream.tellp());
			subresource_data_stream.read(subresources_data.data(), subresource_data_stream.tellp());
			fout.write(subresources_data.data(), subresources_data.size());

			fout.close();
		});

		executor->run(taskflow);

		ImageSpecification texture_spec = {};
		texture_spec.pixels = std::move(bc7_encoded_data);
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