#pragma once 

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include "Asset/AssetType.h"

namespace Omni {

	struct CompressedAssetHeader {
		uint8 header_size;
		AssetType asset_type;
		uint64 uncompressed_data_size;
		uint64 compressed_data_size;
	};

	struct OMNIFORCE_API CompressedAsset {
		CompressedAssetHeader header;
		byte metadata[128];
		uint8* compressed_data;
	};

}