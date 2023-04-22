#pragma once

#include "RendererCommon.h"

#include <vector>

namespace Omni {

	enum class OMNIFORCE_API DescriptorBindingType : uint32 {
		SAMPLED_IMAGE,
		STORAGE_IMAGE,
		UNIFORM_BUFFER,
		STORAGE_BUFFER
	};

	enum class OMNIFORCE_API DescriptorFlags : uint32 {
		PARTIALLY_BOUND = BIT(0)
	};

	struct OMNIFORCE_API DescriptorBinding {
		uint32 binding;
		DescriptorBindingType type;
		uint32 array_count;
		BitMask flags;
	};

	struct OMNIFORCE_API DescriptorSetSpecification {
		std::vector<DescriptorBinding> bindings;
	};

	class OMNIFORCE_API DescriptorSet {
	public:
		static Shared<DescriptorSet> Create(const DescriptorSetSpecification& spec);

		virtual void Destroy() = 0;

		virtual void Write(uint16 binding, uint16 array_element, Shared<Image> image, Shared<ImageSampler> sampler) = 0;
		virtual void Write(uint16 binding, uint16 array_element, Shared<DeviceBuffer> buffer, uint64 size, uint64 offset) = 0;

	private:

	};

}