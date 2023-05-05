#pragma once

#include "VulkanCommon.h"
#include <Renderer/Image.h>

#include <vulkan-memory-allocator-hpp/vk_mem_alloc.h>

namespace Omni {

#pragma region converts
	constexpr VkFormat convert(const ImageFormat& format) {
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

	constexpr ImageFormat convert(const VkFormat& format) {
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
		default:										std::unreachable(); break;;
		}
	}

	constexpr VkImageType convert(const ImageType& type) {
		switch (type)
		{
		case ImageType::TYPE_1D:						return VK_IMAGE_TYPE_1D;
		case ImageType::TYPE_2D:						return VK_IMAGE_TYPE_2D;
		case ImageType::TYPE_3D:						return VK_IMAGE_TYPE_3D;
		default:										std::unreachable(); break;
		}
	}

	constexpr VkImageUsageFlagBits convert(const ImageUsage& usage) {
		switch (usage)
		{
		case ImageUsage::TEXTURE:						return VK_IMAGE_USAGE_SAMPLED_BIT;
		case ImageUsage::RENDER_TARGET:					return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		case ImageUsage::DEPTH_BUFFER:					return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		default:										std::unreachable(); break;
		}
	}

	constexpr VkFilter convert(const SamplerFilteringMode& mode) {
		switch (mode)
		{
		case SamplerFilteringMode::LINEAR:				return VK_FILTER_LINEAR;
		case SamplerFilteringMode::NEAREST:				return VK_FILTER_NEAREST;
		default:										std::unreachable();
		}
	}

	constexpr VkSamplerAddressMode convert(const SamplerAddressMode& mode) {
		switch (mode)
		{
		case SamplerAddressMode::CLAMP:					return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case SamplerAddressMode::CLAMP_BORDER:			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case SamplerAddressMode::REPEAT:				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case SamplerAddressMode::MIRRORED_REPEAT:		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		default:										std::unreachable();
		}
	}

	constexpr VkSamplerMipmapMode convertMipmapMode(const SamplerFilteringMode& mode) {
		switch (mode)
		{
		case SamplerFilteringMode::LINEAR:				return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		case SamplerFilteringMode::NEAREST:				return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		default:										std::unreachable();
		}
	}

#pragma endregion

	class VulkanImage : public Image {
	public:
		VulkanImage();
		VulkanImage(const ImageSpecification& spec, VkImage image, VkImageView view);
		VulkanImage(const ImageSpecification& spec);
		VulkanImage(std::filesystem::path path);
		~VulkanImage();

		void CreateFromRaw(const ImageSpecification& spec, VkImage image, VkImageView view);
		void Destroy() override;

		VkImage Raw() const { return m_Image; }
		VkImageView RawView() const { return m_ImageView; };
		ImageSpecification GetSpecification() const override { return m_Specification; }
		uint32 GetId() const override { return m_Id; }

		VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }

		// It is supposed that image has actually already been transitioned into layout.
		void SetCurrentLayout(VkImageLayout layout) { m_CurrentLayout = layout; } 

	private:
		void CreateTexture();
		void CreateRenderTarget();

	private:
		static uint32 s_IdCounter;

		ImageSpecification m_Specification;
		uint32 m_Id;

		VkImage m_Image;
		VmaAllocation m_Allocation;
		VkImageView m_ImageView;
		VkImageLayout m_CurrentLayout;
		
		bool m_CreatedFromRaw;

	};

	class VulkanImageSampler : public ImageSampler {
	public:
		VulkanImageSampler(const ImageSamplerSpecification& spec);
		~VulkanImageSampler();

		void Destroy() override;

		VkSampler Raw() const { return m_Sampler; }
		ImageSamplerSpecification GetSpecification() const { return m_Specification; }

	private:
		VkSampler m_Sampler;
		ImageSamplerSpecification m_Specification;

	};

}