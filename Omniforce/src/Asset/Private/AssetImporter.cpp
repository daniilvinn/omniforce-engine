#include "../AssetImporter.h"
#include "../AssetType.h"

#include <stb_image.h>

namespace Omni {

	class ImageAssetSourceImporter : public AssetImporter {
	public:
		std::vector<byte> ImporterImplementation(std::filesystem::path path) override {
			stbi_set_flip_vertically_on_load(true);
			int x, y, c;
			stbi_uc* data = stbi_load(path.string().c_str(), &x, &y, &c, STBI_rgb_alpha);
			std::vector<byte> out(data, data + (x * y * STBI_rgb_alpha));
			delete data;
			return out;
		}

		void* GetAdditionalDataImpl(std::filesystem::path path) override {
			ImageSourceAdditionalData* data = new ImageSourceAdditionalData;
			stbi_info(path.string().c_str(), &data->width, &data->height, &data->source_channels);
			return data;
		}

	};

	std::vector<byte> AssetImporter::ImportAssetSource(std::filesystem::path path)
	{
		AssetImporter* impl = nullptr;
		AssetType type = FileExtensionToAssetType(path.extension().string());

		switch (type)
		{
		case AssetType::MESH_SRC:											break;
		case AssetType::AUDIO_SRC:											break;
		case AssetType::IMAGE_SRC: impl = new ImageAssetSourceImporter;		break;
		case AssetType::UNKNOWN:											break;
		default:															break;
		}

		std::vector<byte> data = impl->ImporterImplementation(path);

		delete impl;

		return data;
	}

	void* AssetImporter::GetAdditionalData(std::filesystem::path path)
	{
		AssetImporter* impl = nullptr;
		AssetType type = FileExtensionToAssetType(path.extension().string());

		switch (type)
		{
		case AssetType::MESH_SRC:											break;
		case AssetType::AUDIO_SRC:											break;
		case AssetType::IMAGE_SRC: impl = new ImageAssetSourceImporter;		break;
		case AssetType::UNKNOWN:											break;
		default:															break;
		}


		void* data = impl->GetAdditionalDataImpl(path);
		delete impl;

		return data;
	}

}
