#pragma once 

#include <Foundation/Common.h>
#include <Asset/AssetType.h>

#include <array>
#include <ostream>

namespace Omni {

	/*
	*	Omniforce asset file structure
	*	
	*	File header
	*		header size
	*		asset type
	*		uncompressed data size
	*		subresources size
	*		additional data
	* 
	*	Subresource metadata #0
	*		offset
	*		compressed
	*		size
	*		decompressed size
	*	...
	*	Subresource metadata #15
	*		...
	* 
	*	Resource data	
	*		Subresource #0
	*		...
	*/

	struct AssetFileHeader {
		uint8 header_size = 0;
		AssetType asset_type = AssetType::UNKNOWN;
		uint64 uncompressed_data_size = 0;
		uint64 subresources_size = 0;
		uint64 additional_data = 0;
	};

	struct AssetFileSubresourceMetadata {
		uint32 offset = 0;
		uint32 compressed = 0;
		uint32 size = 0;
		uint32 decompressed_size = 0;
	};

	struct AssetFile {
		AssetFileHeader header;
		std::array<AssetFileSubresourceMetadata, 16> subresources_metadata;
		byte* subresources_data;

		AssetFile()
		{
			header = {};
			subresources_data = nullptr;
		}

		~AssetFile() { Release(); }

		void Release() {
			header = {};
			subresources_metadata.fill({});
			delete[] subresources_data;
		}
	};

	inline std::ostream& operator<< (std::ostream& stream, const AssetFile& file) {
		stream.write((char*)&file.header, sizeof AssetFileHeader);
		stream.write((char*)file.subresources_metadata.data(), sizeof AssetFileSubresourceMetadata * file.subresources_metadata.size());
		stream.write((char*)file.subresources_data, file.header.subresources_size);

		return stream;
	}

}