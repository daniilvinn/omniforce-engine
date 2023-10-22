#include "../AssetCompressor.h"
#include "Core/Utils.h"
#include "Log/Logger.h"

#include <glm/glm.hpp>
#include <bc7enc.h>

namespace Omni {


	void AssetCompressor::Init()
	{
		bc7enc_compress_block_init();
	}

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

	std::vector<byte> AssetCompressor::CompressBC7(const std::vector<RGBA32>& source, uint32 mip0_width, uint32 mip0_height)
	{
		std::vector<byte> compressed_data(source.size()); // actual size in bytes is exactly 4 times smaller
		
		uint32 current_source_width = mip0_width;
		uint32 current_source_height = mip0_height;

		uint32 mip_offset = 0; // offset of current mip in source data array
		bc7enc_compress_block_params params;
		bc7enc_compress_block_params_init(&params);
		bc7enc_compress_block_params_init_linear_weights(&params);

		constexpr int32 BlockSize = 4;

		RGBA32 source_block[BlockSize * BlockSize];

		for (;;) {

			int32 bc_width = current_source_width / BlockSize;
			int32 bc_height = current_source_height / BlockSize;

			#pragma omp parallel for private(source_block)
			for (int32 iy = 0; iy < bc_height; ++iy) {
				for (int32 ix = 0; ix < bc_width; ++ix) {
					int32 px = ix * BlockSize;
					int32 py = iy * BlockSize;
					for (int32 lx = 0; lx < BlockSize; ++lx) {
						for (int32 ly = 0; ly < BlockSize; ++ly) {
							uint32 index = mip_offset + ((px + lx) + (py + ly) * current_source_width);
							source_block[lx + ly * BlockSize] = reinterpret_cast<const RGBA32*>(source.data())[index];
						}
					}
					bc7enc_compress_block(&compressed_data[mip_offset + (16 * (ix + iy * bc_width))], source_block, &params);

				}
			}

			mip_offset += current_source_width * current_source_height;
			current_source_width /= 2;
			current_source_height /= 2;

			if (current_source_width < 4 || current_source_height < 4)
				break;
		}
		
		return compressed_data;

	}

	std::vector<byte> AssetCompressor::CompressGDeflate(const std::vector<byte>& data)
	{
		return {};
	}

}
