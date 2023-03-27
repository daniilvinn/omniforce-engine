#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Memory/Pointers.hpp>

#include "File.h"
#include <filesystem>

namespace Omni {

	enum class FileReadingFlags : uint32 {
		READ_BINARY = BIT(0)
	};

	class OMNIFORCE_API FileSystem {
	public:
		static Shared<File> ReadFile(std::filesystem::path path, const BitMask& flags);
		static void WriteFile(Shared<File> file, std::filesystem::path path, const BitMask& flags);
		static uint64 FileSize(std::filesystem::path path);

		static bool CheckDirectory(std::filesystem::path path);
		static bool CreateDirectory(std::filesystem::path path);
		static bool DestroyDirectory(std::filesystem::path path);

	private:

	};

}