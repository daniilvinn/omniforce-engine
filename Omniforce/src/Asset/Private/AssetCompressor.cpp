#include "../AssetCompressor.h"
#include "Core/Utils.h"
#include "Log/Logger.h"

#include <glm/glm.hpp>

namespace Omni {


	CompressedAsset AssetCompressor::Compress(Shared<AssetBase> asset)
	{
		CompressedAsset compressed_asset = {};
		CompressedAssetHeader header = {};
		header.header_size = sizeof(CompressedAssetHeader);
		header.asset_type = asset->Type;
		header.compressed_data_size = 0;
		header.uncompressed_data_size;

		return compressed_asset;
	}

	Shared<AssetBase> AssetCompressor::Uncompress(CompressedAsset compressed_asset)
	{
		return nullptr;
	}

	uint64 AssetCompressor::ComputeCompressedSize(byte* data, uint64 data_size)
	{
		return 0;
	}

	std::vector<RGBA32> AssetCompressor::GenerateMipMaps(const std::vector<RGBA32>& mip0_data, uint32 image_width, uint32 image_height)
	{
		// create storage for mip levels
		uint32 storage_size = Utils::ComputeMipLevelsStorage<sizeof RGBA32>(image_width, image_height);
		std::vector<RGBA32> storage;
		storage.resize(storage_size / sizeof RGBA32);

		// copy first mip to storage
		memcpy(storage.data(), mip0_data.data(), image_width * image_height * sizeof RGBA32);

		// current image (from which we're computing next level)
		uint32 current_image_width = image_width;
		uint32 current_image_height = image_height;

		// mip to the beginning of current computing (not computed from!) mip's storage
		RGBA32* src_mip_pointer = storage.data();
		RGBA32* dst_mip_pointer = src_mip_pointer + image_width * image_height;

		uint32 src_offset = 0;
		uint32 dst_offset = 0;

		// compute num of mip levels
		uint8 num_mip_levels = Utils::ComputeNumMipLevelsBC7(image_width, image_height);

		// Per mip level
		for (int i = 0; i < num_mip_levels; i++) {
			
			// First y - improve caching
			#pragma omp parallel for
			for (int y = 0; y < current_image_height; y += 2) {
				for (int x = 0; x < current_image_width; x += 2) {
					// Fetch current 2x2 block (left upper pixel)
					RGBA32* current_block = src_mip_pointer + (y * current_image_width + x);

					// Create storage for pixels we're filtering
					glm::uvec4 pixels[4];

					// Fill the data
					pixels[0] = *(current_block);
					pixels[1] = *(current_block + 1);
					pixels[2] = *(current_block + current_image_width);
					pixels[3] = *(current_block + current_image_width + 1);

					glm::uvec4 int_res = (pixels[0] + pixels[1] + pixels[2] + pixels[3]);

					// Compute arithmetical mean of the block
					RGBA32 result =  int_res / 4U;

					// Write results to storage
					uint32 offset = (y / 2 * (current_image_width / 2)) + (x / 2);
					*(dst_mip_pointer + offset) = result;
				}
			}

			// Also update the pointer
			src_mip_pointer += current_image_width * current_image_height;
			dst_mip_pointer += (current_image_width / 2) * (current_image_height / 2);

			// Divide current width and height by two, so it fits to next mip level
			current_image_width /= 2;
			current_image_height /= 2;
		}

		return storage;
	}
}
