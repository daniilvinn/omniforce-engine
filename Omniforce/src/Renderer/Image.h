#pragma once

#include "RendererCommon.h"

#include <filesystem>

namespace Omni {

	enum class OMNIFORCE_API ImageUsage : uint8 {
		TEXTURE,
		RENDER_TARGET,
		DEPTH_BUFFER
	};

	enum class OMNIFORCE_API ImageFormat : uint8 {
		R8,
		RB16,
		RGB24,
		RGBA32,
		BGRA32,
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
			spec.format = ImageFormat::RGBA32;
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
		static Shared<Image> Create(const ImageSpecification& spec);

		virtual ~Image() {}

		virtual void Destroy() = 0;

		virtual ImageSpecification GetSpecification() const = 0;
	};

}