#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"

#include <vector>
#include <filesystem>

namespace Omni {
	
	struct ImageSourceAdditionalData {
		int32 width;
		int32 height;
		int32 source_channels;
	};

	// TODO
	class OMNIFORCE_API AssetImporter {
	public:
		std::vector<byte> ImportAssetSource(std::filesystem::path path);
		void* GetAdditionalData(std::filesystem::path path);

	protected:
		virtual std::vector<byte> ImporterImplementation(std::filesystem::path path) { return {}; };
		virtual void* GetAdditionalDataImpl(std::filesystem::path path) { return {}; };
	};

	

}