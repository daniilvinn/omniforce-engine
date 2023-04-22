#include "../Image.h"

#include <Platform/Vulkan/VulkanImage.h>

namespace Omni {

	Shared<Image> Image::Create(const ImageSpecification& spec)
	{
		return std::make_shared<VulkanImage>(spec);
	}

	Shared<ImageSampler> ImageSampler::Create(const ImageSamplerSpecification& spec)
	{
		return std::make_shared<VulkanImageSampler>(spec);
	}

}