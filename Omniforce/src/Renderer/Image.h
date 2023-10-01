#pragma once

#include "RendererCommon.h"
#include "PipelineStage.h"

#include "Core/UUID.h"

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
		D32
	};

	enum class OMNIFORCE_API ImageType : uint8 {
		TYPE_1D,
		TYPE_2D,
		TYPE_3D,
	};

	struct OMNIFORCE_API ImageSpecification {
		uvec3 extent;
		std::filesystem::path path;
		ImageFormat format;
		ImageUsage usage;
		ImageType type;
		uint8 array_layers;
		uint8 mip_levels;

		static ImageSpecification Default() {
			ImageSpecification spec;
			spec.extent = { 0, 0, 0 };
			spec.path = "";
			spec.format = ImageFormat::RGBA32_SRGB;
			spec.usage = ImageUsage::TEXTURE;
			spec.type = ImageType::TYPE_2D;
			spec.mip_levels = 1;
			spec.array_layers = 1;

			return spec;
		};
	};

	class OMNIFORCE_API Image
	{
	public:
		static Shared<Image> Create(const ImageSpecification& spec, const UUID& id = UUID());

		virtual ~Image() {}

		virtual void Destroy() = 0;

		virtual ImageSpecification GetSpecification() const = 0;
		virtual void SetLayout(
			Shared<DeviceCmdBuffer> cmd_buffer,
			ImageLayout new_layout,
			PipelineStage src_stage,
			PipelineStage dst_stage,
			PipelineAccess src_access = PipelineAccess::NONE,
			PipelineAccess dst_access = PipelineAccess::NONE
 		) = 0;
		virtual UUID GetId() const = 0;
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