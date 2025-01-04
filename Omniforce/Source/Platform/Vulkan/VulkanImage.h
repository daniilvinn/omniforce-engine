#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <Renderer/Image.h>

#include <vk_mem_alloc.h>

namespace Omni {

	class VulkanImage : public Image {
	public:
		VulkanImage();
		VulkanImage(const ImageSpecification& spec, VkImage image, VkImageView view);
		VulkanImage(const ImageSpecification& spec, AssetHandle id);
		VulkanImage(const ImageSpecification& spec, const std::vector<RGBA32> data, const AssetHandle& id);
		VulkanImage(std::filesystem::path path);
		~VulkanImage();

		void CreateFromRaw(const ImageSpecification& spec, VkImage image, VkImageView view);
		void Destroy() override;

		VkImage Raw() const { return m_Image; }
		VkImageView RawView() const { return m_ImageView; };
		ImageSpecification GetSpecification() const override { return m_Specification; }

		ImageLayout GetCurrentLayout() const { return m_CurrentLayout; }

		// It is supposed that image has actually already been transitioned into layout.
		void SetCurrentLayout(ImageLayout layout) { m_CurrentLayout = layout; } 
		void SetLayout(Shared<DeviceCmdBuffer> cmd_buffer, ImageLayout new_layout, PipelineStage src_stage, PipelineStage dst_stage, BitMask src_access = 0, BitMask dst_access = 0) override;

	private:
		void CreateTexture();
		void CreateRenderTarget();
		void CreateDepthBuffer();
		void CreateStorageImage();

	private:

		ImageSpecification m_Specification;

		VkImage m_Image;
		VmaAllocation m_Allocation;
		VkImageView m_ImageView;
		ImageLayout m_CurrentLayout;
		
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

#pragma region converts
	constexpr VkFormat convert(const ImageFormat& format) {
		switch (format)
		{
		case ImageFormat::R8:							return VK_FORMAT_R8_SRGB;
		case ImageFormat::RB16:							return VK_FORMAT_R8G8_SRGB;
		case ImageFormat::RGB24:						return VK_FORMAT_R8G8B8_SRGB;
		case ImageFormat::RGBA32_SRGB:					return VK_FORMAT_R8G8B8A8_SRGB;
		case ImageFormat::RGBA32_UNORM:					return VK_FORMAT_R8G8B8A8_UNORM;
		case ImageFormat::BGRA32_SRGB:					return VK_FORMAT_B8G8R8A8_SRGB;
		case ImageFormat::BGRA32_UNORM:					return VK_FORMAT_B8G8R8A8_UNORM;
		case ImageFormat::RGB32_HDR:					return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case ImageFormat::RGBA64_HDR:					return VK_FORMAT_R16G16B16A16_SFLOAT;
		case ImageFormat::RGBA128_HDR:					return VK_FORMAT_R32G32B32A32_SFLOAT;
		case ImageFormat::D32:							return VK_FORMAT_D32_SFLOAT;
		case ImageFormat::BC7:							return VK_FORMAT_BC7_UNORM_BLOCK;
		case ImageFormat::BC1:							return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case ImageFormat::BC5:							return VK_FORMAT_BC5_UNORM_BLOCK;
		case ImageFormat::BC6h:							return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case ImageFormat::RGBA64_SFLOAT:				return VK_FORMAT_R16G16B16A16_SFLOAT;
		case ImageFormat::RGB24_UNORM:					return VK_FORMAT_R8G8B8_UNORM;
		case ImageFormat::R64_UINT:						return VK_FORMAT_R64_UINT;
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
		case VK_FORMAT_R8G8B8A8_SRGB:					return ImageFormat::RGBA32_SRGB;
		case VK_FORMAT_B8G8R8A8_SRGB:					return ImageFormat::BGRA32_SRGB;
		case VK_FORMAT_B8G8R8A8_UNORM:					return ImageFormat::BGRA32_UNORM;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:			return ImageFormat::RGB32_HDR;
		case VK_FORMAT_R16G16B16A16_SFLOAT:				return ImageFormat::RGBA64_HDR;
		case VK_FORMAT_R32G32B32A32_SFLOAT:				return ImageFormat::RGBA128_HDR;
		case VK_FORMAT_D32_SFLOAT:						return ImageFormat::D32;
		case VK_FORMAT_R16G16B16_SFLOAT: 				return ImageFormat::RGBA64_SFLOAT;
		case VK_FORMAT_R8G8B8_UNORM: 					return ImageFormat::RGB24_UNORM;
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

}