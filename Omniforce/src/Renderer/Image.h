#pragma once

#include "ForwardDecl.h"
#include <Foundation/Types.h>

#include <filesystem>

namespace Omni {

	enum class OMNIFORCE_API ImageUsage : uint8 {
		SAMPLED,
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
		uvec2 extent;
		std::filesystem::path path;
		ImageFormat format;
		ImageUsage usage;
		ImageType type;
	};

	class OMNIFORCE_API Image {
	public:
		static Shared<Image> Create(const ImageSpecification& spec);

		virtual ~Image() {}
		virtual void Destroy() = 0;

		virtual ImageSpecification GetSpecification() const = 0;
	};

}