#pragma once

#include "Core/UUID.h"
#include "Asset/AssetBase.h"
#include "RendererCommon.h"
#include "PipelineStage.h"

#include <filesystem>

namespace Omni {

	// =======
	//  Image
	// =======

	enum class ImageLayout {
		UNDEFINED = 0,
		GENERAL = 1,
		COLOR_ATTACHMENT = 2,
		DEPTH_STENCIL_ATTACHMENT = 3,
		DEPTH_STENCIL_READ_ONLY = 4,
		SHADER_READ_ONLY = 5,
		TRANSFER_SRC = 6,
		TRANSFER_DST = 7,
		PRESENT_SRC = 1000001002
	};

	enum class OMNIFORCE_API ImageUsage : uint8 {
		TEXTURE,
		RENDER_TARGET,
		DEPTH_BUFFER
	};

	enum class OMNIFORCE_API ImageFormat : uint8 {
		R8,
		RB16,
		RGB24,
		RGBA32_SRGB,
		RGBA32_UNORM,
		BGRA32_SRGB,
		BGRA32_UNORM,
		RGB32_HDR,
		RGBA64_HDR,
		RGBA128_HDR,
		D32,
		BC1,
		BC5,
		BC6h,
		BC7,
		RGBA64_SFLOAT,
		RGB24_UNORM
	};

	enum class OMNIFORCE_API ImageType : uint8 {
		TYPE_1D,
		TYPE_2D,
		TYPE_3D,
	};

	struct OMNIFORCE_API ImageSpecification {
		uvec3 extent;
		std::vector<byte> pixels;
		std::filesystem::path path;
		ImageFormat format;
		ImageUsage usage;
		ImageType type;
		uint8 array_layers;
		uint8 mip_levels;
		OMNI_DEBUG_ONLY_FIELD(std::string debug_name);

		static ImageSpecification Default() {
			ImageSpecification spec;
			spec.extent = { 0, 0, 0 };
			spec.format = ImageFormat::RGBA32_SRGB;
			spec.usage = ImageUsage::TEXTURE;
			spec.type = ImageType::TYPE_2D;
			spec.mip_levels = 1;
			spec.array_layers = 1;
			OMNI_DEBUG_ONLY_CODE(spec.debug_name = "");

			return spec;
		};
	};

	class OMNIFORCE_API Image : public AssetBase
	{
	public:
		static Shared<Image> Create(const ImageSpecification& spec, const AssetHandle& id = AssetHandle());
		static Shared<Image> Create(const ImageSpecification& spec, const std::vector<RGBA32> data, const AssetHandle& id = AssetHandle());

		virtual ~Image() {}

		virtual void Destroy() = 0;

		virtual ImageSpecification GetSpecification() const = 0;
		virtual void SetLayout(
			Shared<DeviceCmdBuffer> cmd_buffer,
			ImageLayout new_layout,
			PipelineStage src_stage,
			PipelineStage dst_stage,
			BitMask src_access = 0,
			BitMask dst_access = 0
 		) = 0;

	protected:
		Image(AssetHandle handle) {
			Handle = handle;
			Type = AssetType::OMNI_IMAGE;
		};
	};


	// ===============
	//	Image sampler
	// ===============
	enum class OMNIFORCE_API SamplerFilteringMode : uint32 {
		NEAREST,
		LINEAR
	};

	enum class OMNIFORCE_API SamplerAddressMode : uint32 {
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP,
		CLAMP_BORDER
	};

	struct OMNIFORCE_API ImageSamplerSpecification {
		SamplerFilteringMode min_filtering_mode;
		SamplerFilteringMode mag_filtering_mode;
		SamplerFilteringMode mipmap_filtering_mode;
		SamplerAddressMode address_mode;
		float min_lod;
		float max_lod;
		float lod_bias;
		uint8 anisotropic_filtering_level;
	};

	class OMNIFORCE_API ImageSampler {
	public:
		static Shared<ImageSampler> Create(const ImageSamplerSpecification& spec);

		virtual void Destroy() = 0;
	};

}