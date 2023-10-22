#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Memory/Pointers.hpp>

#include "AssetBase.h"
#include "CompressedAsset.h"

namespace Omni {

	class OMNIFORCE_API AssetCompressor {
	public:
		/*
		*  @brief compresses asset using GDEFLATE (level 12) algorithm. Images are additionaly encoded to BC7 format.
		*  @return struct with header, metadata and compressed data.
		*/
		static void Init();

		static CompressedAsset Compress(Shared<AssetBase> asset);

		/*
		*  @brief uncompresses data. If possibly, uses GPU decompression provided by VK_NV_memory_decompression extension.
		*  @return pointer to created asset.
		*/
		static Shared<AssetBase> Uncompress(CompressedAsset compressed_asset);

		/*
		*  @brief Generates mip levels.Discards last two levels with 1 - or 2 - pixel wide dimensions(because mips are further used for BC7 encoding)
		*/ 
		static std::vector<RGBA32> GenerateMipMaps(const std::vector<RGBA32>& mip0_data, uint32 image_width, uint32 image_height);

		/*
		*  @brief  Encodes RGBA32 image to BC7 image.
		*  @return Array of 128-bit values, representing blocks
		*/
		static std::vector<byte> CompressBC7(const std::vector<RGBA32>& source, uint32 image_width, uint32 image_height);

		/*
		*  @brief Compresses data using NVIDIA GDeflate algorithm.
		*  @return stream of compressed pages
		*/
		static std::vector<byte> CompressGDeflate(const std::vector<byte>& data);

	};

}