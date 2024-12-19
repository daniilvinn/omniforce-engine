#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"

#include <vector>
#include <filesystem>

namespace Omni {
	
	struct ImageSourceMetadata {
		int32 width;
		int32 height;
		int32 source_channels;
	};

	// TODO
	class OMNIFORCE_API ImageSourceImporter {
	public:
		std::vector<byte> ImportFromSource(stdfs::path path);
		void ImportFromSource(std::vector<byte>* out,stdfs::path path);
		void ImportFromMemory(std::vector<byte>* out, const std::vector<byte>& in);
		ImageSourceMetadata* GetMetadata(stdfs::path path);
		ImageSourceMetadata* GetMetadataFromMemory(const std::vector<byte>& in);

	};

	

}