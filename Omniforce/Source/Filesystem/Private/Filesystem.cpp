#include <Foundation/Common.h>
#include <Filesystem/Filesystem.h>

#include <stb_image.h>

namespace Omni {

	struct FileSystemData {
		std::filesystem::path working_directory = std::filesystem::current_path();
	} s_FileSystemData;

	Ref<File> FileSystem::ReadFile(IAllocator* allocator, std::filesystem::path path, const BitMask& flags)
	{
		byte* data;
		uint64 file_size = 0;

		std::ifstream reader(path, std::ifstream::ate | std::ifstream::binary);

		file_size = reader.tellg();
		data = new byte[file_size];
		
		reader.seekg(0);
		reader.read((char*)data, file_size);
		reader.close();

		return CreateRef<File>(allocator, data, file_size, flags, path);;
	}

	uint64 FileSystem::FileSize(std::filesystem::path path)
	{
		uint64 file_size = 0;

		std::ifstream reader(path, std::ifstream::ate | std::ifstream::binary);
		file_size = reader.tellg();
		reader.close();

		return file_size;
	}

	

	void FileSystem::WriteFile(Ref<File> file, std::filesystem::path path, const BitMask& flags)
	{

	}

	bool FileSystem::CheckDirectory(std::filesystem::path path)
	{
		return std::filesystem::exists(path);
	}

	bool FileSystem::CreateDirectory(std::filesystem::path path)
	{
		return std::filesystem::create_directories(path);
	}

	bool FileSystem::DestroyDirectory(std::filesystem::path path)
	{
		return std::filesystem::remove(path);
	}

	void FileSystem::SetWorkingDirectory(std::filesystem::path path)
	{
		s_FileSystemData.working_directory = path;
	}

	std::filesystem::path FileSystem::GetWorkingDirectory()
	{
		return s_FileSystemData.working_directory;
	}

	std::filesystem::path FileSystem::GetTextureDirectory()
	{
		return GetWorkingDirectory() / "assets" / "textures";
	}

	std::filesystem::path FileSystem::GetAudioDirectory()
	{
		return {};
	}

	std::filesystem::path FileSystem::GetScriptDirectory()
	{
		return {};
	}

}