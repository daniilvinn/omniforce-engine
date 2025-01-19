#include <Foundation/Common.h>
#include <Asset/Importers/ImageImporter.h>

#include <Asset/AssetType.h>

#include <stb_image.h>

namespace Omni {

	std::vector<byte> ImageSourceImporter::ImportFromSource(stdfs::path path)
	{
		stbi_set_flip_vertically_on_load(true);
		int x, y, c;
		stbi_uc* data = stbi_load(path.string().c_str(), &x, &y, &c, STBI_rgb_alpha);
		std::vector<byte> out(data, data + (x * y * STBI_rgb_alpha));
		delete data;
		return out;
	}

	void ImageSourceImporter::ImportFromSource(std::vector<byte>* out, std::filesystem::path path)
	{
		stbi_set_flip_vertically_on_load(true);
		ImageSourceMetadata* metadata = (ImageSourceMetadata*)GetMetadata(path);

		out->resize(metadata->width * metadata->height * metadata->source_channels);

		int x, y, c;
		stbi_uc* data = stbi_load(path.string().c_str(), &x, &y, &c, STBI_rgb_alpha);

		memcpy(out->data(), data, out->size());

		delete data;
		delete metadata;
	}

	void ImageSourceImporter::ImportFromMemory(std::vector<byte>* out, const std::vector<byte>& in)
	{
		stbi_set_flip_vertically_on_load(true);
		auto* metadata = GetMetadataFromMemory(in);

		out->resize(metadata->width * metadata->height * 4);
		int x, y, c;
		stbi_uc* data = stbi_load_from_memory(in.data(), in.size(), &x, &y, &c, STBI_rgb_alpha);
		memcpy(out->data(), data, out->size());
		delete data;
		delete metadata;
	}

	ImageSourceMetadata* ImageSourceImporter::GetMetadata(std::filesystem::path path)
	{
		ImageSourceMetadata* data = new ImageSourceMetadata;
		stbi_info(path.string().c_str(), &data->width, &data->height, &data->source_channels);
		return data;
	}

	ImageSourceMetadata* ImageSourceImporter::GetMetadataFromMemory(const std::vector<byte>& in)
	{
		ImageSourceMetadata* data = new ImageSourceMetadata;
		stbi_info_from_memory(in.data(), in.size(), &data->width, &data->height, &data->source_channels);
		return data;
	}

}
