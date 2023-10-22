#pragma once

#include "Foundation/Macros.h"
#include "Foundation/Types.h"

#include <vector>
#include <filesystem>

namespace Omni {
	

	// TODO
	class AssetImporter {
	public:

		std::vector<byte> ImportImage(std::filesystem::path path);

	};

}