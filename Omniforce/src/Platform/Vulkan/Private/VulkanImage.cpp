#include "../VulkanImage.h"
#include "../VulkanDeviceBuffer.h"
#include "../VulkanGraphicsContext.h"
#include "../VulkanDeviceCmdBuffer.h"
#include "VulkanMemoryAllocator.h"

#include <Renderer/Renderer.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Omni {

	VulkanImage::VulkanImage()
		: m_Specification(), m_Image(VK_NULL_HANDLE), m_ImageView(VK_NULL_HANDLE), 
		m_Allocation(VK_NULL_HANDLE), m_CurrentLayout(ImageLayout::UNDEFINED), m_CreatedFromRaw(false) 
	{
	}

	VulkanImage::VulkanImage(const ImageSpecification& spec, UUID id)
		: VulkanImage()
	{
 		m_Specification = spec;
		m_Id = id;

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

	void VulkanImage::SetLayout(Shared<DeviceCmdBuffer> cmd_buffer, ImageLayout new_layout, PipelineStage src_stage, PipelineStage dst_stage, PipelineAccess src_access, PipelineAccess dst_access)
	{
		Shared<VulkanDeviceCmdBuffer> vk_cmd_buffer = ShareAs<VulkanDeviceCmdBuffer>(cmd_buffer);

		VkImageMemoryBarrier image_memory_barrier = {};
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.image = m_Image;
		image_memory_barrier.oldLayout = (VkImageLayout)m_CurrentLayout;
		image_memory_barrier.newLayout = (VkImageLayout)new_layout;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.srcAccessMask = (VkAccessFlags)src_access;
		image_memory_barrier.dstAccessMask = (VkAccessFlags)dst_access;
		image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(vk_cmd_buffer->Raw(),
			(VkPipelineStageFlags)src_stage,
			(VkPipelineStageFlags)dst_stage,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&image_memory_barrier
		);

		m_CurrentLayout = new_layout;
	}

	void VulkanImage::CreateTexture()
	{
		stbi_set_flip_vertically_on_load(true);

		int image_width, image_height, channel_count;
		byte* image_data = stbi_load(m_Specification.path.string().c_str(), &image_width, &image_height, &channel_count, STBI_rgb_alpha);
		m_Specification.extent = { (uint32)image_width, (uint32)image_height };

		VkImageCreateInfo texture_create_info = {};
		texture_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		texture_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
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
		staging_buffer_spec.size = image_width * image_height * STBI_rgb_alpha;
		staging_buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
		staging_buffer_spec.buffer_usage = DeviceBufferUsage::STAGING_BUFFER;

		VulkanDeviceBuffer staging_buffer(staging_buffer_spec, image_data, image_width * image_height * STBI_rgb_alpha);
		
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

		m_CurrentLayout = ImageLayout::SHADER_READ_ONLY;

		stbi_image_free(image_data);

		VkImageViewCreateInfo image_view_create_info = {};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.image = m_Image;
		image_view_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
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
		m_Specification.type = ImageType::TYPE_2D;
		m_Specification.usage = ImageUsage::TEXTURE;
		m_Specification.format = ImageFormat::RGBA32_UNORM;

		staging_buffer.Destroy();
	}

	void VulkanImage::CreateRenderTarget()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VkImageCreateInfo render_target_create_info = {};
		render_target_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		render_target_create_info.extent = { m_Specification.extent.x, m_Specification.extent.y, 1 };
		render_target_create_info.imageType = VK_IMAGE_TYPE_2D;
		render_target_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		render_target_create_info.mipLevels = 1;
		render_target_create_info.arrayLayers = 1;
		render_target_create_info.format = convert(m_Specification.format);
		render_target_create_info.samples = VK_SAMPLE_COUNT_1_BIT; // HACK
		render_target_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		render_target_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		render_target_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		auto allocator = VulkanMemoryAllocator::Get();
		m_Allocation = allocator->AllocateImage(&render_target_create_info, 0, &m_Image);

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

		Shared<DeviceCmdBuffer> cmd_buffer = DeviceCmdBuffer::Create(
			DeviceCmdBufferLevel::PRIMARY, 
			DeviceCmdBufferType::TRANSIENT, 
			DeviceCmdType::GENERAL
		);

		cmd_buffer->Begin();
		SetLayout(
			cmd_buffer,
			ImageLayout::COLOR_ATTACHMENT,
			PipelineStage::TOP_OF_PIPE,
			PipelineStage::COLOR_ATTACHMENT_OUTPUT,
			PipelineAccess::NONE,
			PipelineAccess::COLOR_ATTACHMENT_WRITE
		);
		cmd_buffer->End();
		cmd_buffer->Execute(true);
		cmd_buffer->Destroy();
	}

	void VulkanImage::CreateFromRaw(const ImageSpecification& spec, VkImage image, VkImageView view)
	{
		m_Image = image;
		m_ImageView = view;
		m_Specification = spec;
	}


	/*
	*	SAMPLER METHODS' DEFINITIONS
	*/

	VulkanImageSampler::VulkanImageSampler(const ImageSamplerSpecification& spec)
		: m_Sampler(VK_NULL_HANDLE), m_Specification(spec)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VkSamplerCreateInfo sampler_create_info = {};
		sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_create_info.minFilter = convert(spec.min_filtering_mode);
		sampler_create_info.magFilter = convert(spec.mag_filtering_mode);
		sampler_create_info.mipmapMode = convertMipmapMode(spec.mipmap_filtering_mode);
		sampler_create_info.addressModeU = convert(spec.address_mode);
		sampler_create_info.addressModeV = convert(spec.address_mode);
		sampler_create_info.addressModeW = convert(spec.address_mode);
		sampler_create_info.anisotropyEnable = spec.anisotropic_filtering_level == 1.0f ? VK_FALSE : VK_TRUE;
		sampler_create_info.maxAnisotropy = (float32)spec.anisotropic_filtering_level;
		sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler_create_info.unnormalizedCoordinates = VK_FALSE;
		sampler_create_info.compareEnable = VK_FALSE;
		sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler_create_info.mipLodBias = spec.lod_bias;
		sampler_create_info.minLod = spec.min_lod;
		sampler_create_info.maxLod = spec.max_lod;

		VK_CHECK_RESULT(vkCreateSampler(device->Raw(), &sampler_create_info, nullptr, &m_Sampler));
	}

	VulkanImageSampler::~VulkanImageSampler()
	{

	}

	void VulkanImageSampler::Destroy()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();
		vkDestroySampler(device->Raw(), m_Sampler, nullptr);
		m_Sampler = VK_NULL_HANDLE;
	}

}