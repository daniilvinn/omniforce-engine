#pragma once

#include <Foundation/Common.h>
#include <Renderer/RendererCommon.h>

namespace Omni {

	enum class OMNIFORCE_API DescriptorBindingRate {
		SCENE = 0,
		PASS = 1,
		DRAW_CALL = 2
	};

	enum class OMNIFORCE_API DescriptorBindingType : uint32 {
		SAMPLED_IMAGE,
		STORAGE_IMAGE,
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		ACCELERATION_STRUCTURE
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
		static Ref<DescriptorSet> Create(IAllocator* allocator, const DescriptorSetSpecification& spec);
		virtual ~DescriptorSet() {};

		virtual void Write(uint16 binding, uint16 array_element, Ref<Image> image, Ref<ImageSampler> sampler) = 0;
		virtual void Write(uint16 binding, uint16 array_element, Ref<DeviceBuffer> buffer, uint64 size, uint64 offset) = 0;

	private:

	};

}