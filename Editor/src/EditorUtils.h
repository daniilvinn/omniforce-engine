#pragma once

#include <Asset/Importers/ImageImporter.h>
#include <Asset/Importers/ModelImporter.h>
#include <Asset/AssetManager.h>
#include <Asset/AssetBase.h>
#include <Asset/AssetCompressor.h>
#include <Asset/OFRController.h>
#include <Threading/JobSystem.h>

#include <vector>

namespace EditorUtils {

	using namespace Omni;

	inline AssetHandle ConvertImage(std::filesystem::path path, std::filesystem::path out, bool import = true) {
		ImageSourceImporter importer;
		std::vector<byte> data = importer.ImportFromSource(path);
		ImageSourceMetadata* additional_data = importer.GetMetadata(path);
		std::vector<RGBA32> mip_maps = AssetCompressor::GenerateMipMaps({ data.begin(), data.end() }, additional_data->width, additional_data->height);

		data = AssetCompressor::CompressBC7(mip_maps, additional_data->width, additional_data->height, Utils::ComputeNumMipLevelsBC7(additional_data->width, additional_data->height) + 1);

		ImageSpecification image_spec = ImageSpecification::Default();
		image_spec.extent = { (uint32)additional_data->width, (uint32)additional_data->height, 1 };
		image_spec.path = path;
		image_spec.mip_levels = Utils::ComputeNumMipLevelsBC7(additional_data->width, additional_data->height) + 1;
		image_spec.pixels = data;
		image_spec.type = ImageType::TYPE_2D;
		image_spec.usage = ImageUsage::TEXTURE;
		image_spec.format = ImageFormat::BC7;
		image_spec.array_layers = 1;

		Shared<AssetBase> image = Image::Create(image_spec, 0);

		auto executor = JobSystem::GetExecutor();

		tf::Taskflow taskflow;
		taskflow.emplace([=]() {
			uint64 ofr_additional_data = (uint64)additional_data->width | (uint64)additional_data->height << 32;

			uint32 mip_levels = Utils::ComputeNumMipLevelsBC7(additional_data->width, additional_data->height) + 1;
			uint32 current_subresource_offset = 0;
			std::array<AssetFileSubresourceMetadata, 16> metadata = {};

			for (int32 i = 0; i < mip_levels; i++) {
				uint32 current_mip_size = (additional_data->width >> i) * (additional_data->height >> i);
				metadata[i].decompressed_size = current_mip_size;
				metadata[i].offset = current_subresource_offset;
				current_subresource_offset += current_mip_size;
			}

			OFRController ofr_controller(out);
			ofr_controller.Build(metadata, data, ofr_additional_data);

			delete additional_data;
		});

		executor->run(taskflow);

		if (!import)
			image->Destroy();

		return import ? AssetManager::Get()->RegisterAsset(image) : AssetHandle(0);
	}

	inline AssetHandle ImportAndConvertMesh() {

	};

}