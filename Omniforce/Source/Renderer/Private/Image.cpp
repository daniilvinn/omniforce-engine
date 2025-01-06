#include <Foundation/Common.h>
#include <Renderer/Image.h>

#include <Platform/Vulkan/VulkanImage.h>

namespace Omni {

	Ref<Image> Image::Create(IAllocator* allocator, const ImageSpecification& spec, const AssetHandle& id /* = UUID()*/)
	{
		return CreateRef<VulkanImage>(allocator, spec, id);
	}

	Ref<Image> Image::Create(IAllocator* allocator, const ImageSpecification& spec, const std::vector<RGBA32> data, const AssetHandle& id)
	{
		return CreateRef<VulkanImage>(allocator, spec, data, id);
	}

	Ref<ImageSampler> ImageSampler::Create(IAllocator* allocator, const ImageSamplerSpecification& spec)
	{
		return CreateRef<VulkanImageSampler>(allocator, spec);
	}

}