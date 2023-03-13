#include "../VulkanImage.h"

namespace Cursed {

	VulkanImage::VulkanImage()
		: m_Specification(), m_Image(VK_NULL_HANDLE), m_ImageView(VK_NULL_HANDLE) {}

	VulkanImage::VulkanImage(const ImageSpecification& spec)
	{

	}


	VulkanImage::VulkanImage(const ImageSpecification& spec, VkImage image, VkImageView view)
	{
		m_Image = image;
		m_ImageView = view;
		m_Specification = spec;
	}

	VulkanImage::~VulkanImage()
	{
		
	}

	void VulkanImage::Destroy()
	{
		throw std::logic_error("Method is not implemented");
	}

	void VulkanImage::CreateFromRaw(const ImageSpecification& spec, VkImage image, VkImageView view)
	{
		m_Image = image;
		m_ImageView = view;
		m_Specification = spec;
	}

}