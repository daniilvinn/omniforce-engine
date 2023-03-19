#pragma once

#include "VulkanCommon.h"

#include <Renderer/Image.h>

namespace Cursed {

	constexpr VkFormat CursedToVulkanImageFormat(const ImageFormat& format) {
		switch (format)
		{
		case ImageFormat::R8:							return VK_FORMAT_R8_SRGB;
		case ImageFormat::RB16:							return VK_FORMAT_R8G8_SRGB;
		case ImageFormat::RGB24:						return VK_FORMAT_R8G8B8_SRGB;
		case ImageFormat::RGBA32:						return VK_FORMAT_R8G8B8A8_SRGB;
		case ImageFormat::BGRA32:						return VK_FORMAT_B8G8R8A8_SRGB;
		case ImageFormat::RGB32_HDR:					return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case ImageFormat::RGBA64_HDR:					return VK_FORMAT_R16G16B16A16_SFLOAT;
		case ImageFormat::RGBA128_HDR:					return VK_FORMAT_R32G32B32A32_SFLOAT;
		case ImageFormat::D32:							return VK_FORMAT_D32_SFLOAT;
		default:
			std::unreachable();
			break;
		}
	}

	constexpr ImageFormat VulkanToCursedImageFormat(const VkFormat& format) {
		switch (format) {
		case VK_FORMAT_R8_SRGB:							return ImageFormat::R8;
		case VK_FORMAT_R8G8_SRGB:						return ImageFormat::RB16;
		case VK_FORMAT_R8G8B8_SRGB:						return ImageFormat::RGB24;
		case VK_FORMAT_R8G8B8A8_SRGB:					return ImageFormat::RGBA32;
		case VK_FORMAT_B8G8R8A8_SRGB:					return ImageFormat::BGRA32;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:			return ImageFormat::RGB32_HDR;
		case VK_FORMAT_R16G16B16A16_SFLOAT:				return ImageFormat::RGBA64_HDR;
		case VK_FORMAT_R32G32B32A32_SFLOAT:				return ImageFormat::RGBA128_HDR;
		case VK_FORMAT_D32_SFLOAT:						return ImageFormat::D32;
		default:
			std::unreachable();
			break;
		}
	}

	class VulkanImage : public Image {
	public:
		VulkanImage();
		VulkanImage(const ImageSpecification& spec, VkImage image, VkImageView view);
		VulkanImage(const ImageSpecification& spec);
		~VulkanImage();

		void CreateFromRaw(const ImageSpecification& spec, VkImage image, VkImageView view);
		void Destroy() override;

		VkImage Raw() const { return m_Image; }
		VkImageView RawView() const { return m_ImageView; }
		ImageSpecification GetSpecification() const { return m_Specification; }

		VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }

		// It is supposed that image has actually already been transitioned into layout.
		void SetCurrentLayout(VkImageLayout layout) { m_CurrentLayout = layout; } 
		

	private:
		ImageSpecification m_Specification;

		VkImage m_Image;
		VkImageView m_ImageView;
		VkImageLayout m_CurrentLayout;
		

	};

}