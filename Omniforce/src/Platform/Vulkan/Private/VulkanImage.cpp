#include "../VulkanImage.h"
#include "../VulkanDeviceBuffer.h"
#include "../VulkanGraphicsContext.h"
#include "VulkanMemoryAllocator.h"

#include <Renderer/Renderer.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Omni {

	VulkanImage::VulkanImage()
		: m_Specification(), m_Image(VK_NULL_HANDLE), m_ImageView(VK_NULL_HANDLE), m_Allocation(VK_NULL_HANDLE), m_CurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED),
		  m_CreatedFromRaw(false) {}

	VulkanImage::VulkanImage(const ImageSpecification& spec)
		: VulkanImage()
	{
		m_Specification = spec;

		switch (m_Specification.usage)
		{
		case ImageUsage::TEXTURE:			this->CreateTexture();			break;
		case ImageUsage::RENDER_TARGET:		this->CreateRenderTarget();		break;
		case ImageUsage::DEPTH_BUFFER:		OMNIFORCE_ASSERT(false);		break;
		default:							std::unreachable();				break;
		}
	}

	VulkanImage::VulkanImage(const ImageSpecification& spec, VkImage image, VkImageView view)
		: VulkanImage()
	{
		m_CreatedFromRaw = true;
		this->CreateFromRaw(spec, image, view);
	}

	VulkanImage::VulkanImage(std::filesystem::path path)
		: VulkanImage()
	{
		
	}

	VulkanImage::~VulkanImage()
	{
		this->Destroy();
	}

	void VulkanImage::Destroy()
	{
		if (m_CreatedFromRaw) return;

		auto device = VulkanGraphicsContext::Get()->GetDevice();
		auto allocator = VulkanMemoryAllocator::Get();

		vkDestroyImageView(device->Raw(), m_ImageView, nullptr);
		allocator->DestroyImage(&m_Image, &m_Allocation);

		m_ImageView = VK_NULL_HANDLE;
	}

	void VulkanImage::CreateTexture()
	{
		int image_width, image_height, channel_count;
		byte* image_data = stbi_load(m_Specification.path.string().c_str(), &image_width, &image_height, &channel_count, STBI_rgb_alpha);

		VkImageCreateInfo texture_create_info = {};
		texture_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		texture_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		texture_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		texture_create_info.imageType = VK_IMAGE_TYPE_2D;
		texture_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		texture_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		texture_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		texture_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		texture_create_info.arrayLayers = 1;
		texture_create_info.mipLevels = log2(std::max(image_width, image_height)) + 1;
		texture_create_info.extent = { (uint32)image_width, (uint32)image_height, 1 };

		auto allocator = VulkanMemoryAllocator::Get();
		m_Allocation = allocator->AllocateImage(&texture_create_info, 0, &m_Image);

		DeviceBufferSpecification staging_buffer_spec = {};
		staging_buffer_spec.size = image_width * image_height * channel_count;
		staging_buffer_spec.memory_usage = DeviceBufferMemoryUsage::ONE_TIME_HOST_ACCESS;
		staging_buffer_spec.buffer_usage = DeviceBufferUsage::STAGING_BUFFER;

		VulkanDeviceBuffer staging_buffer(staging_buffer_spec, image_data, image_width * image_height * channel_count);
		
		VkBufferImageCopy buffer_image_copy = {};
		buffer_image_copy.imageExtent = { (uint32)image_width, (uint32)image_height, 1 };
		buffer_image_copy.bufferOffset = 0;
		buffer_image_copy.imageOffset = { 0, 0, 0 };
		buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		buffer_image_copy.imageSubresource.baseArrayLayer = 0;
		buffer_image_copy.imageSubresource.layerCount = 1;
		buffer_image_copy.imageSubresource.mipLevel = 0;
		buffer_image_copy.bufferRowLength = 0;
		buffer_image_copy.bufferImageHeight = 0;

		auto device = VulkanGraphicsContext::Get()->GetDevice();
		VkCommandBuffer cmd_buffer = device->AllocateTransientCmdBuffer();
		
		// So here we need to load and transition layout of all mip-levels of a texture
		// Firstly we transition all of them into transfer destination layout
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_Image;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = texture_create_info.mipLevels;

		vkCmdPipelineBarrier(cmd_buffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);

		vkCmdCopyBufferToImage(cmd_buffer, staging_buffer.Raw(), m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
		for (int i = 0; i < texture_create_info.mipLevels - 1; i++) 
		{
			// Transition previous mip to transfer source layout
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseMipLevel = i;

			vkCmdPipelineBarrier(cmd_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier
			);

			if (i >= texture_create_info.mipLevels) break;
			
			VkImageBlit image_blit_params = {};
			image_blit_params.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_blit_params.srcSubresource.baseArrayLayer = 0;
			image_blit_params.srcSubresource.layerCount = 1;
			image_blit_params.srcSubresource.mipLevel = i;
			image_blit_params.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_blit_params.dstSubresource.baseArrayLayer = 0;
			image_blit_params.dstSubresource.layerCount = 1;
			image_blit_params.dstSubresource.mipLevel = i + 1;
			image_blit_params.srcOffsets[0] = { 0, 0, 0 };
			image_blit_params.srcOffsets[1] = { image_width, image_height, 1 };
			image_blit_params.dstOffsets[0] = { 0, 0, 0 };
			image_blit_params.dstOffsets[1] = { image_width / 2, image_height / 2, 1 };

			vkCmdBlitImage(cmd_buffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit_params, VK_FILTER_LINEAR);

			image_width /= 2;
			image_height /= 2;
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = texture_create_info.mipLevels - 1;

		// last image is TRANSFER_DST_OPTIMAL, so need to transition it separately
		VkImageMemoryBarrier last_mip_level_barrier = {};
		last_mip_level_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		last_mip_level_barrier.image = m_Image;
		last_mip_level_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		last_mip_level_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		last_mip_level_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		last_mip_level_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		last_mip_level_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		last_mip_level_barrier.subresourceRange.baseArrayLayer = 0;
		last_mip_level_barrier.subresourceRange.layerCount = 1;
		last_mip_level_barrier.subresourceRange.baseMipLevel = texture_create_info.mipLevels - 1;
		last_mip_level_barrier.subresourceRange.levelCount = 1;

		VkImageMemoryBarrier final_barriers[2] = { barrier, last_mip_level_barrier };

		vkCmdPipelineBarrier(cmd_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			2,
			final_barriers
		);

		device->ExecuteTransientCmdBuffer(cmd_buffer);

		m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		stbi_image_free(image_data);

		VkImageViewCreateInfo image_view_create_info = {};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.image = m_Image;
		image_view_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = texture_create_info.mipLevels;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		VK_CHECK_RESULT(vkCreateImageView(device->Raw(), &image_view_create_info, nullptr, &m_ImageView));

		m_Specification.array_layers = 1;
		m_Specification.mip_levels = texture_create_info.mipLevels;
		m_Specification.extent = {(uint32)image_width, (uint32)image_height};
		m_Specification.type = ImageType::TYPE_2D;
		m_Specification.usage = ImageUsage::TEXTURE;
		m_Specification.format = ImageFormat::RGBA32;
	}

	void VulkanImage::CreateRenderTarget()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VkImageCreateInfo render_target_create_info = {};
		render_target_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		render_target_create_info.extent = { m_Specification.extent.x, m_Specification.extent.y };
		render_target_create_info.imageType = VK_IMAGE_TYPE_2D;
		render_target_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		render_target_create_info.mipLevels = 1;
		render_target_create_info.arrayLayers = 1;
		render_target_create_info.format = convert(m_Specification.format);
		render_target_create_info.samples = VK_SAMPLE_COUNT_1_BIT; // HACK
		render_target_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		render_target_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		render_target_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		auto allocator = VulkanMemoryAllocator::Get();
		allocator->AllocateImage(&render_target_create_info, 0, &m_Image);

		VkImageViewCreateInfo image_view_create_info = {};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.image = m_Image;
		image_view_create_info.format = convert(m_Specification.format);
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		VK_CHECK_RESULT(vkCreateImageView(device->Raw(), &image_view_create_info, nullptr, &m_ImageView));

		VkImageMemoryBarrier image_memory_barrier = {};
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.image = m_Image;
		image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.levelCount = 1;


		VkCommandBuffer cmd_buffer = device->AllocateTransientCmdBuffer();
		vkCmdPipelineBarrier(
			cmd_buffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&image_memory_barrier
		);
		device->ExecuteTransientCmdBuffer(cmd_buffer);
	}

	void VulkanImage::CreateFromRaw(const ImageSpecification& spec, VkImage image, VkImageView view)
	{
		m_Image = image;
		m_ImageView = view;
		m_Specification = spec;
	}

}