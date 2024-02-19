#include "../Importers/MaterialImporter.h">

#include <Asset/AssetManager.h>
#include <Asset/Material.h>
#include <Asset/Importers/ImageImporter.h>
#include <Asset/AssetCompressor.h>
#include <Core/Utils.h>
#include <Threading/JobSystem.h>

namespace Omni {

	glm::vec4 c(std::array<float32, 4> in) {
		return { in[0], in[1], in[2], in[3] };
	}

	template<>
	void MaterialImporter::HandleProperty<ftf::Optional<ftf::TextureInfo>>(std::string_view key, const ftf::Optional<ftf::TextureInfo>& property, Shared<Material> material, const ftf::Asset& root, std::shared_mutex& mutex)
	{
		if (!property.has_value())
			return;

		AssetHandle image_handle = LoadTextureProperty(property.value().textureIndex, root);

		mutex.lock();
		material->AddProperty(key, image_handle);
		material->AddShaderMacro(fmt::format("__OMNI_HAS_{}", key));
		mutex.unlock();
	}

	template<>
	void MaterialImporter::HandleProperty<ftf::Optional<ftf::NormalTextureInfo>>(std::string_view key, const ftf::Optional<ftf::NormalTextureInfo>& property, Shared<Material> material, const ftf::Asset& root, std::shared_mutex& mutex)
	{
		if (!property.has_value())
			return;

		AssetHandle image_handle = LoadTextureProperty(property.value().textureIndex, root);

		mutex.lock();
		material->AddProperty(key, image_handle);
		material->AddShaderMacro(fmt::format("__OMNI_HAS_{}", key));
		// TODO: add scale
		//if (property.value().scale != 1.0f) {
		//	material->AddShaderMacro()
		//	material->AddProperty("NORMAL_MAP_SCALE", property.value().scale);
		//}
		mutex.unlock();
	}

	template<>
	void MaterialImporter::HandleProperty<ftf::Optional<ftf::OcclusionTextureInfo>>(std::string_view key, const ftf::Optional<ftf::OcclusionTextureInfo>& property, Shared<Material> material, const ftf::Asset& root, std::shared_mutex& mutex)
	{
		if (!property.has_value())
			return;

		AssetHandle image_handle = LoadTextureProperty(property.value().textureIndex, root);

		mutex.lock();
		material->AddProperty(key, image_handle);
		material->AddShaderMacro(fmt::format("__OMNI_HAS_{}", key));
		mutex.unlock();
	}

	AssetHandle MaterialImporter::LoadTextureProperty(uint64 texture_index, const ftf::Asset& root)
	{
		const auto& ftf_texture = root.textures[texture_index];

		if (!ftf_texture.imageIndex.has_value())
			return 0;

		const uint64 ftf_image_index = ftf_texture.imageIndex.value();
		const auto& ftf_image = root.images[ftf_image_index];
		const auto& ftf_image_data = ftf_image.data;

		std::vector<byte> image_data;
		uint32 image_width, image_height;

		std::visit(ftf::visitor{
					[](auto& arg) {},
					[&](const ftf::sources::URI filepath) {
						ImageSourceImporter image_importer;
						ImageSourceMetadata* image_metadata = (ImageSourceMetadata*)image_importer.GetMetadata(filepath.uri.path());
						image_data.resize(image_metadata->width * image_metadata->height * image_metadata->source_channels);

						image_importer.ImportFromSource(&image_data, filepath.uri.path());

						image_width = image_metadata->width;
						image_height = image_metadata->height;

						delete image_metadata;
					},
					[&](const ftf::sources::Vector& vector) {
						ImageSourceImporter image_importer;
						ImageSourceMetadata* image_metadata = image_importer.GetMetadataFromMemory(vector.bytes);

						image_data.resize(image_metadata->width * image_metadata->height * 4);

						image_importer.ImportFromMemory(&image_data, vector.bytes);

						image_width = image_metadata->width;
						image_height = image_metadata->height;

						delete image_metadata;
					}
			},
			ftf_image_data
		);

		AssetCompressor compressor;

		std::vector<RGBA32> intermediate_storage (image_data.size() / sizeof RGBA32);
		memcpy(intermediate_storage.data(), image_data.data(), image_data.size());

		std::vector<RGBA32> mip_mapped_image = compressor.GenerateMipMaps(
			intermediate_storage,
			image_width,
			image_height
		);

		uint8 mip_levels_count = Utils::ComputeNumMipLevelsBC7(image_width, image_height) + 1;
		image_data = compressor.CompressBC7({ mip_mapped_image.begin(), mip_mapped_image.end() }, image_width, image_height, mip_levels_count);

		ImageSpecification image_spec = ImageSpecification::Default();
		image_spec.extent = { image_width, image_height };
		image_spec.format = ImageFormat::BC7;
		image_spec.pixels = std::move(image_data);
		image_spec.mip_levels = mip_levels_count;

		AssetHandle asset_handle = AssetManager::Get()->RegisterAsset(Image::Create(image_spec));

		return asset_handle;
	}

	AssetHandle MaterialImporter::Import(const ftf::Asset& root, const ftf::Material& in_material)
	{
		AssetHandle id = rh::hash<std::string>()(in_material.name.c_str());

		Shared<Material> material = Material::Create(in_material.name.c_str(), id);

		AssetManager* asset_manager = AssetManager::Get();
		asset_manager->RegisterAsset(material, id);

		if (in_material.pbrData.baseColorTexture.has_value())
			material->AddShaderMacro("__OMNI_SHADING_MODEL_PBR");
		//else
		//	material->AddShaderMacro("__OMNI_SHADING_MODEL_NON_PBR");

		std::shared_mutex mutex;

		JobSystem* js = JobSystem::Get();

		js->Execute([&]() { HandleProperty("ALPHA_CUTOFF", in_material.alphaCutoff, material, root, mutex); });
		if (in_material.alphaMode == ftf::AlphaMode::Mask)
			js->Execute([&]() { HandleProperty("ALPHA_MASK", 1, material, root, mutex); });

		js->Execute([&]() { HandleProperty("BASE_COLOR_FACTOR", c(in_material.pbrData.baseColorFactor), material, root, mutex); });
		js->Execute([&]() { HandleProperty("METALLIC_FACTOR", in_material.pbrData.metallicFactor, material, root, mutex); });
		js->Execute([&]() { HandleProperty("ROUGHNESS_FACTOR", in_material.pbrData.roughnessFactor, material, root, mutex); });

		js->Execute([&]() { HandleProperty("BASE_COLOR_MAP", in_material.pbrData.baseColorTexture, material, root, mutex); });
		js->Execute([&]() { HandleProperty("METALLIC_ROUGHNESS_MAP", in_material.pbrData.metallicRoughnessTexture, material, root, mutex); });

		js->Execute([&]() { HandleProperty("NORMAL_MAP", in_material.normalTexture, material, root, mutex); });
		js->Execute([&]() { HandleProperty("OCCLUSION_MAP", in_material.occlusionTexture, material, root, mutex); });

		js->Wait();

		return id;
	}



}