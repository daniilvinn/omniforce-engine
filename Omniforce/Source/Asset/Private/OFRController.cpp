#include <Foundation/Common.h>
#include <Asset/OFRController.h>

namespace Omni {

	std::unique_ptr<AssetFile> OFRController::Build(std::array<AssetFileSubresourceMetadata, 16> metadata, std::vector<byte> data, uint64 additional_data) {
		m_Dirty = true;

		AssetFileHeader file_header = {};
		file_header.header_size = sizeof AssetFileHeader;
		file_header.additional_data = additional_data;
		file_header.asset_type = AssetType::OMNI_IMAGE;
		file_header.uncompressed_data_size = data.size();
		file_header.subresources_size = 0;

		uint8 num_subresources = 0;
		for (int32 i = 0; i < metadata.size(); i++) {
			if (!metadata[i].decompressed_size)
				break;
			num_subresources++;
		}
		std::stringstream subresource_data_stream;

		for (uint8 i = 0; i < num_subresources; i++) {
			uint32 subresource_size = metadata[i].decompressed_size;

			auto subresource_source_data_begin = data.begin() + metadata[i].offset;
			auto subresource_source_data_end = subresource_source_data_begin + subresource_size;

			metadata[i].compressed = subresource_size > (65536 / 2);
			metadata[i].offset = file_header.subresources_size;

			if (metadata[i].compressed)
				metadata[i].size = AssetCompressor::CompressGDeflate({ subresource_source_data_begin, subresource_source_data_end }, &subresource_data_stream);
			else {
				subresource_data_stream.write((char*)&(*subresource_source_data_begin), subresource_size);
				metadata[i].size = subresource_size;
			}

			file_header.subresources_size += metadata[i].size;
		}

		std::vector<byte> subresources_data_vector(subresource_data_stream.tellp());
		uint32 tellp = subresource_data_stream.tellp();
		subresource_data_stream.read((char*)subresources_data_vector.data(), tellp);

		std::unique_ptr<AssetFile> file = std::make_unique<AssetFile>();
		file->header = file_header;
		memcpy(file->subresources_metadata.data(), metadata.data(), sizeof AssetFileSubresourceMetadata * file->subresources_metadata.size());
		file->subresources_data = new byte[file_header.subresources_size];
		memcpy(file->subresources_data, subresources_data_vector.data(), subresources_data_vector.size());

		m_FileStream << *file;

		return file;
	}

	bool OFRController::DestroyOfflineStorage()
	{
		m_FileStream.close();
		m_Header = {};
		m_Metadata = {};
		m_Dirty = true;
		return std::filesystem::remove(m_Filepath);
	}

	std::vector<Omni::byte> OFRController::ExtractSubresources(uint32 first, uint32 last)
	{
		assert(first < last);

		if (Dirty()) LoadHeaderAndMetadata();

		std::stringstream subresources_stream;

		auto position = GetOFTHeaderSize() + GetMaxSubresources() * sizeof AssetFileSubresourceMetadata + m_Metadata[first].offset;

		m_FileStream.seekg(position);

		for (int i = first; i < last; i++) {
			if (!m_Metadata[i].size)
				break;

			if (m_Metadata[i].compressed)
				AssetCompressor::DecompressGDeflateCPU(&m_FileStream, m_Metadata[i].size, &subresources_stream);
			else {
				std::vector<byte> staging(m_Metadata[i].size);
				m_FileStream.read((char*)staging.data(), staging.size());
				subresources_stream.write((const char*)staging.data(), staging.size());
			}
		}

		std::vector<byte> out(subresources_stream.tellp());
		subresources_stream.read((char*)out.data(), m_Header.uncompressed_data_size);

		return out;
	}

	Omni::uint32 OFRController::GetNumSubresources()
	{
		if (m_Dirty) {
			LoadHeaderAndMetadata();
		}

		for (int i = 0; i < m_Metadata.size(); i++) {
			if (!m_Metadata[i].decompressed_size)
				return i;
		}
		return 16; // All 16 subresources are used
	}

}