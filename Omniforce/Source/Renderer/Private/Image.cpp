#include <Foundation/Common.h>
#include <Renderer/Image.h>

#include <Platform/Vulkan/VulkanImage.h>

namespace Omni {

	Shared<Image> Image::Create(const ImageSpecification& spec, const AssetHandle& id /* = UUID()*/)
	{
		return std::make_shared<VulkanImage>(spec, id);
	}

	Shared<Image> Image::Create(const ImageSpecification& spec, const std::vector<RGBA32> data, const AssetHandle& id)
	{
		return std::make_shared<VulkanImage>(spec, data, id);
	}

	Shared<ImageSampler> ImageSampler::Create(const ImageSamplerSpecification& spec)
	{
		return std::make_shared<VulkanImageSampler>(spec);
	}

}