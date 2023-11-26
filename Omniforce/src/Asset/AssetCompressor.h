#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Memory/Pointers.hpp>

#include "AssetBase.h"
#include "AssetFile.h"

#include <iostream>

namespace Omni {

	/*
	*  @brief Singleton for asset compression functionality.
	*/
	class OMNIFORCE_API AssetCompressor {
	public:

		/*
		*  @brief Generates mip levels.Discards last two levels with 1 - or 2 - pixel wide dimensions(because mips are further used for BC7 encoding)
		*/ 
		static std::vector<RGBA32> GenerateMipMaps(const std::vector<RGBA32>& mip0_data, uint32 image_width, uint32 image_height);

		/*
		*  @brief Encodes RGBA32 image to BC7 image.
		*  @return Array of 128-bit values, representing blocks
		*/
		static std::vector<byte> CompressBC7(const std::vector<RGBA32>& source, uint32 image_width, uint32 image_height);

		/*
		*  @brief Compresses data using NVIDIA GDeflate algorithm.
		*  @return Size of compressed data
		*  @param[out] out: data stream to write compressed data in
		*/
		static uint32 CompressGDeflate(const std::vector<byte>& data, std::ostream* out);

		/*
		*  @brief Decompresses data using single core CPU GDeflate decompression implementation.
		*  @param[in] in: input stream of data
		*  @param[out] out: vector of bytes
		*/
		static uint64 DecompressGDeflateCPU(std::istream* in, uint64 size, std::ostream* out);

		/*
		*  @brief Updates given CRC32 checksum for a given stream of data
		*  @return Updated CRC32 checksum
		*/
		static uint32 GDeflateCRC32(uint32 crc32, const std::vector<byte>& data);

		inline static constexpr uint32 GDEFLATE_PAGE_SIZE = 65536;
		inline static constexpr uint32 GDEFLATE_END_OF_PAGES_TAG = 1;
		
		struct GDeflatePageHeader {
			size_t compressed_size = 0;
		};

	private:

		inline static constexpr GDeflatePageHeader EndOfPagesTag = { 0 };

	};

}