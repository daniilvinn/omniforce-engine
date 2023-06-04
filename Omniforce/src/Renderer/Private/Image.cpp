#include "../Image.h"

#include <Platform/Vulkan/VulkanImage.h>

namespace Omni {

	Shared<Image> Image::Create(const ImageSpecification& spec, const UUID& id /* = UUID()*/)
	{
		return std::make_shared<VulkanImage>(spec, id);
	}

	Shared<ImageSampler> ImageSampler::Create(const ImageSamplerSpecification& spec)
	{
		return std::make_shared<VulkanImageSampler>(spec);
	}

}