#pragma once

#include <Foundation/Common.h>
#include <Asset/AssetCompressor.h>
#include <Asset/AssetFile.h>

namespace Omni {

class OMNIFORCE_API OFRController {
public:
	OFRController(std::filesystem::path path, uint64 offset = 0)
		: m_Filepath(path), m_OriginalOffset(0), m_Dirty(true)
	{
		if (!std::filesystem::exists(path))
		{
			m_FileStream.open(path, std::fstream::out);
			m_FileStream.close();
		}

		m_FileStream.open(path, std::fstream::binary | std::fstream::in | std::fstream::out);

		m_FileStream.seekg(offset);
		m_FileStream.seekp(offset);

	}

	Ptr<AssetFile> Build(std::array<AssetFileSubresourceMetadata, 16> metadata, std::vector<byte> data, uint64 additional_data);

	bool DestroyOfflineStorage();

	/*
	*  @brief Extracts all subresources from OFR file.
	*/
	std::vector<byte> ExtractSubresources() {
		return ExtractSubresources(0, GetNumSubresources());
	}

	// It is application user's responsibility take make sure that subresource with such index exists.
	std::vector<byte> ExtractSubresource(uint32 index) {
		assert(index < 16);
		return ExtractSubresources(index, index + 1);
	}

	// It is application user's responsibility take make sure that subresource with such index exists.
	std::vector<byte> ExtractSubresources(uint32 first, uint32 last);

	AssetFileHeader ExtractHeader() {
		if (m_Dirty) LoadHeaderAndMetadata();
		return m_Header;
	}

	std::array<AssetFileSubresourceMetadata, 16> ExtractMetadata() {
		if (m_Dirty) LoadHeaderAndMetadata();
		return m_Metadata;
	}

	// Helper functions
	inline static uint32 GetOFTHeaderSize() { return sizeof AssetFileHeader; }

	inline static uint32 GetMaxSubresources() { return AssetFile().subresources_metadata.size(); }

	uint32 GetNumSubresources();

	/*
	*  Dirty bit flag check
	*/
	inline bool Dirty() const { return m_Dirty; }

private:
	void LoadHeaderAndMetadata() {
		uint32 current_p = m_FileStream.tellg();
		m_FileStream.seekg(m_OriginalOffset);
		m_FileStream.read((char*)&m_Header, sizeof AssetFileHeader);
		m_FileStream.read((char*)m_Metadata.data(), sizeof AssetFileSubresourceMetadata * m_Metadata.size());
		m_FileStream.seekg(current_p);

		m_Dirty = false;
	}

private:
	// Precached data
	std::fstream m_FileStream;
	uint64 m_OriginalOffset;
	std::filesystem::path m_Filepath;
	AssetFileHeader m_Header;
	std::array<AssetFileSubresourceMetadata, 16> m_Metadata;
	bool m_Dirty;
	// Data maybe too large, so we don't hold it in memory

};

}