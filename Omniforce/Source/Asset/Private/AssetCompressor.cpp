#include <Foundation/Common.h>
#include <Asset/AssetCompressor.h>

#include <Core/Utils.h>
#include <Threading/JobSystem.h>

#include <glm/glm.hpp>
#include <rdo_bc_encoder.h>
#include <libdeflate.h>

namespace Omni {

	static constexpr size_t kGDeflatePageSize = 65536;
	// The maximum number of GDeflate pages to process per batch
	static constexpr size_t kMaxPagesPerBatch = 512;
	// The size of intermediate buffer storage required
	static constexpr size_t kIntermediateBufferSize = kMaxPagesPerBatch * kGDeflatePageSize;

	std::ostream& operator<< (std::ostream& stream, const libdeflate_gdeflate_out_page& page) {
		AssetCompressor::GDeflatePageHeader pageHeader{ static_cast<uint32_t>(page.nbytes) };
		stream.write(reinterpret_cast<const char*>(&pageHeader), sizeof(pageHeader));
		stream.write(static_cast<char*>(page.data), page.nbytes);

		return stream;
	}

	std::istream& operator>> (std::istream& stream, libdeflate_gdeflate_in_page& page) {
		AssetCompressor::GDeflatePageHeader header;
		stream.read(reinterpret_cast<char*>(&header), sizeof header);

		page.nbytes = header.compressed_size;
		stream.read(reinterpret_cast<char*>(const_cast<void*>(page.data)), header.compressed_size);

		return stream;
	}

	std::istream& operator>> (std::istream& stream, AssetCompressor::GDeflatePageHeader& header) {
		stream.read(reinterpret_cast<char*>(&header), sizeof header);
		return stream;
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
			tf::Taskflow taskflow;
			
			// Calculate the number of rows to process
			uint32 num_rows = current_image_height / 2;
			
			for (uint32 row_idx = 0; row_idx < num_rows; row_idx++) {
				taskflow.emplace([&, row_idx, current_image_width, current_image_height, src_mip_pointer, dst_mip_pointer]() {
					int y = row_idx * 2;
					for (int x = 0; x < current_image_width; x += 2) {
						// Fetch current 2x2 block (left upper pixel)
						RGBA32* current_block = src_mip_pointer + (y * current_image_width + x);

						// Create storage for pixels we're filtering
						glm::uvec4 pixels[4] = {};

						// Fill the data
						pixels[0] = *(current_block);
						pixels[1] = *(current_block + 1);
						pixels[2] = *(current_block + current_image_width);
						pixels[3] = *(current_block + current_image_width + 1);

						glm::uvec4 int_res = (pixels[0] + pixels[1] + pixels[2] + pixels[3]);

						// Compute arithmetical mean of the block
						glm::uvec4 result = int_res / 4U;

						// Write results to storage
						uint32 offset = (y / 2 * (current_image_width / 2)) + (x / 2);
						*(dst_mip_pointer + offset) = result;
					}
				});
			}
			
			// Execute all tasks and wait for completion
			JobSystem::GetExecutor()->run(taskflow).wait();

			// Also update the pointer
			src_mip_pointer += current_image_width * current_image_height;
			dst_mip_pointer += (current_image_width / 2) * (current_image_height / 2);

			// Divide current width and height by two, so it fits to next mip level
			current_image_width >>= 1;
			current_image_height >>= 1;
		}

		return storage;
	}

	std::vector<Omni::byte> AssetCompressor::CompressBC7(const std::vector<RGBA32>& source, uint32 image_width, uint32 image_height, uint8 mip_levels_count)
	{
		std::vector<byte> output_data(image_width * image_height * mip_levels_count);
		uint32 current_mip_offset = 0;
		for(int i = 0; i < mip_levels_count; i++) {
			uint32 current_mip_size = image_width * image_height;

			utils::image_u8 image_data(image_width, image_height);
			memcpy(image_data.get_pixels().data(), source.data() + current_mip_offset, image_width * image_height * 4);

			rdo_bc::rdo_bc_encoder bc7_encoder;
			rdo_bc::rdo_bc_params encoder_params;
			bc7_encoder.init(image_data, encoder_params);

			bc7_encoder.encode();
			memcpy(output_data.data() + current_mip_offset, bc7_encoder.get_blocks(), current_mip_size);

			bc7_encoder.clear();
			image_data.clear();

			current_mip_offset += current_mip_size;
			image_width >>= 1;
			image_height >>= 1;
		}
		return output_data;
	}

	uint32 AssetCompressor::CompressGDeflate(const std::vector<byte>& data, std::ostream* out)
	{
		auto c = libdeflate_alloc_gdeflate_compressor(12);

		if (c == nullptr)
			return 0;

		uint32   compressed_size = 0;
		uint32_t crc32 = 0;

		std::vector<char>                         compressed(kIntermediateBufferSize);
		std::vector<libdeflate_gdeflate_out_page> pages(kIntermediateBufferSize / kGDeflatePageSize + 1);

		// Calculate the number of GDEFLATE pages and the amount of intermediate
		// memory required
		size_t npages;
		size_t compBound = libdeflate_gdeflate_compress_bound(c, data.size(), &npages);
		size_t pageCompBound = compBound / npages;

		// Make sure compressed data fits.
		// NOTE: compress bound value can be larger than the uncompressed data size!
		compressed.resize(compBound);
		pages.resize(npages);

		// Initialize output page table
		uint32_t pageIdx = 0;
		for (auto& page : pages)
		{
			page.data = compressed.data() + pageIdx * pageCompBound;
			page.nbytes = pageCompBound;
			++pageIdx;
		}

		// Compress pages
		if (libdeflate_gdeflate_compress(c, data.data(), data.size(), pages.data(), npages))
		{
			// Gather and write compressed pages to output stream
			for (auto& page : pages)
			{
				*out << page;
				compressed_size += page.nbytes + sizeof page.nbytes;
			}

			// Update rolling crc32
			crc32 = libdeflate_crc32(crc32, data.data(), data.size());
		}
		else { return 0; }


		libdeflate_free_gdeflate_compressor(c);

		return compressed_size;
	}

	uint64 AssetCompressor::DecompressGDeflateCPU(std::istream* input_stream, uint64 size, std::ostream* out_stream)
	{
		if (!input_stream->good() || !out_stream->good()) return 0;

		auto d = libdeflate_alloc_gdeflate_decompressor();

		if (d == nullptr) return 0;

		std::vector<char> in(kGDeflatePageSize);
		std::vector<char> out(kGDeflatePageSize);
		uint32_t          crc32 = 0;
		auto initial_streampos = input_stream->tellg();
		uint32 offset = input_stream->tellg() - initial_streampos;


		while (offset != size)
		{
			// Read a compressed page header
			AssetCompressor::GDeflatePageHeader page_header;
			*input_stream >> page_header;

			// Make sure page data fits
			in.resize(page_header.compressed_size);

			// Read page data
			input_stream->read(in.data(), in.size());

			// Decompress page
			libdeflate_gdeflate_in_page page{ in.data(), in.size() };
			size_t                      outSize = 0;
			libdeflate_gdeflate_decompress(d, &page, 1, out.data(), out.size(), &outSize);

			// Update rolling crc32
			crc32 = libdeflate_crc32(crc32, out.data(), outSize);

			// Write uncompressed data
			out_stream->write(out.data(), outSize);

			offset = input_stream->tellg() - initial_streampos;
		}

		libdeflate_free_gdeflate_decompressor(d);

		return true;

	}

	uint32 AssetCompressor::GDeflateCRC32(uint32 crc32, const std::vector<byte>& data)
	{
		return libdeflate_crc32(crc32, data.data(), data.size());
	}




}
