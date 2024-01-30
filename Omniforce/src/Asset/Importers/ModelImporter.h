#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <filesystem>

namespace Omni {

	class OMNIFORCE_API ModelImporter {
	public:
		AssetHandle Import(std::filesystem::path path);

	private:

	};

}