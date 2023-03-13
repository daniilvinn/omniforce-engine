#pragma once

#include "ForwardDecl.h"
#include <Foundation/Types.h>

#include <filesystem>

namespace Cursed {

	enum class CURSED_API ImageUsage : uint32 {
		SAMPLED,
		RENDER_TARGET,
		DEPTH_BUFFER
	};

	enum class CURSED_API ImageFormat : uint32 {
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

	enum class CURSED_API ImageType : uint32 {
		TYPE_1D,
		TYPE_2D,
		TYPE_3D,
	};

	struct CURSED_API ImageSpecification {
		uvec2 extent;
		std::filesystem::path path;
		ImageFormat format;
		ImageUsage usage;
		ImageType type;
	};

	class CURSED_API Image {
	public:
		static Shared<Image> Create(const ImageSpecification& spec);

		virtual ~Image() {}

		virtual void Destroy() = 0;
	};

}