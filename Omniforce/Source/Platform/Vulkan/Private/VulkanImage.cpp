#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanImage.h>

#include <Core/RuntimeExecutionContext.h>
#include <Platform/Vulkan/VulkanDeviceBuffer.h>
#include <Platform/Vulkan/VulkanGraphicsContext.h>
#include <Platform/Vulkan/VulkanDeviceCmdBuffer.h>
#include <Platform/Vulkan/Private/VulkanMemoryAllocator.h>
#include <Renderer/Renderer.h>

#include <stb_image.h>
#include <bc7enc.h>

namespace Omni {

	VulkanImage::VulkanImage()
		: Image(0), m_Specification(), m_Image(VK_NULL_HANDLE), m_ImageView(VK_NULL_HANDLE),
		m_Allocation(VK_NULL_HANDLE), m_CurrentLayout(ImageLayout::UNDEFINED), m_CreatedFromRaw(false) 
	{
	}

	VulkanImage::VulkanImage(const ImageSpecification& spec, AssetHandle id)
		: VulkanImage()
	{
		Handle = id;
 		m_Specification = spec;

		switch (m_Specification.usage)
		{
		case ImageUsage::TEXTURE:			this->CreateTexture();			break;
		case ImageUsage::RENDER_TARGET:		this->CreateRenderTarget();		break;
		case ImageUsage::DEPTH_BUFFER:		this->CreateDepthBuffer();		break;
		case ImageUsage::STORAGE_IMAGE:		this->CreateStorageImage();		break;
		default:							std::unreachable();				break;
		}

		OMNI_DEBUG_ONLY_CODE(
			VkDebugUtilsObjectNameInfoEXT name_info = {};
			name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			name_info.objectType = VK_OBJECT_TYPE_IMAGE;
			name_info.objectHandle = (uint64)m_Image;
			name_info.pObjectName = spec.debug_name.c_str();

			vkSetDebugUtilsObjectNameEXT(VulkanGraphicsContext::Get()->GetDevice()->Raw(), &name_info);

			VkDebugUtilsObjectNameInfoEXT view_name_info = {};
			view_name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			view_name_info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
			view_name_info.objectHandle = (uint64)m_ImageView;
			view_name_info.pObjectName = fmt::format("{} view", spec.debug_name.c_str()).c_str();

			vkSetDebugUtilsObjectNameEXT(VulkanGraphicsContext::Get()->GetDevice()->Raw(), &view_name_info);
			VulkanMemoryAllocator::Get()->SetAllocationName(m_Allocation, m_Specification.debug_name);
		);
		
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

	VulkanImage::VulkanImage(const ImageSpecification& spec, const std::vector<RGBA32> data, const AssetHandle& id)
		: VulkanImage()
	{

	}

	VulkanImage::~VulkanImage()
	{
		if (m_CreatedFromRaw) return;

		auto view = m_ImageView;
		auto allocation = m_Allocation;
		auto image = m_Image;
		auto device = VulkanGraphicsContext::Get()->GetDevice()->Raw();

		RuntimeExecutionContext::Get().GetObjectLifetimeManager().EnqueueObjectDeletion(
			[view, allocation, image, device]() {
				auto allocator = VulkanMemoryAllocator::Get();

				vkDestroyImageView(device, view, nullptr);
				allocator->DestroyImage(image, allocation);
			}
		);

		m_ImageView = VK_NULL_HANDLE;
	}

	void VulkanImage::SetLayout(Ref<DeviceCmdBuffer> cmd_buffer, ImageLayout new_layout, PipelineStage src_stage, PipelineStage dst_stage, BitMask src_access, BitMask dst_access)
	{
		WeakPtr<VulkanDeviceCmdBuffer> vk_cmd_buffer = cmd_buffer;

		VkImageMemoryBarrier2 image_memory_barrier = {};
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		image_memory_barrier.image = m_Image;
		image_memory_barrier.oldLayout = (VkImageLayout)m_CurrentLayout;
		image_memory_barrier.newLayout = (VkImageLayout)new_layout;
		image_memory_barrier.srcStageMask = (BitMask)src_stage;
		image_memory_barrier.dstStageMask = (BitMask)dst_stage;
		image_memory_barrier.srcAccessMask = (VkAccessFlags)src_access;
		image_memory_barrier.dstAccessMask = (VkAccessFlags)dst_access;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.subresourceRange.aspectMask = m_Specification.usage == ImageUsage::DEPTH_BUFFER ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.levelCount = 1;

		VkDependencyInfo dep_info = {};
		dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dep_info.imageMemoryBarrierCount = 1;
		dep_info.pImageMemoryBarriers = &image_memory_barrier;

		vkCmdPipelineBarrier2(vk_cmd_buffer->Raw(),
			&dep_info
		);

		m_CurrentLayout = new_layout;
	}

	void VulkanImage::CreateTexture()
	{
		VkImageCreateInfo texture_create_info = {};
		texture_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		texture_create_info.format = convert(m_Specification.format);
		texture_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		texture_create_info.imageType = VK_IMAGE_TYPE_2D;
		texture_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		texture_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		texture_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		texture_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		texture_create_info.arrayLayers = 1;
		texture_create_info.mipLevels = m_Specification.mip_levels;
		texture_create_info.extent = { m_Specification.extent.x, m_Specification.extent.y, 1 };

		auto allocator = VulkanMemoryAllocator::Get();
		m_Allocation = allocator->AllocateImage(&texture_create_info, 0, &m_Image);

		DeviceBufferSpecification staging_buffer_spec = {};
		staging_buffer_spec.size = m_Specification.pixels.size();
		staging_buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
		staging_buffer_spec.buffer_usage = DeviceBufferUsage::STAGING_BUFFER;

		VulkanDeviceBuffer staging_buffer(staging_buffer_spec, m_Specification.pixels.data(), m_Specification.pixels.size());

		auto device = VulkanGraphicsContext::Get()->GetDevice();
		VulkanDeviceCmdBuffer cmd_buffer = device->AllocateTransientCmdBuffer();

		// So here we need to load and transition layout of all mip-levels of a texture
		// Firstly we transition all of them into transfer destination layout
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_Image;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = texture_create_info.mipLevels;

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

		// Compute transfer regions
		std::vector<VkBufferImageCopy> copy_regions(m_Specification.mip_levels);
		uint32 buffer_offset = 0;

		for (int i = 0; i < copy_regions.size(); i++) {
			uvec2 mip_size = { m_Specification.extent.x / (uint32)(std::pow(2, i)), m_Specification.extent.y / (uint32)(std::pow(2, i)) };

			VkBufferImageCopy& buffer_image_copy = copy_regions[i];
			buffer_image_copy.imageExtent = { mip_size.x, mip_size.y, 1 };
			buffer_image_copy.bufferOffset = buffer_offset;
			buffer_image_copy.imageOffset = { 0, 0, 0 };
			buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			buffer_image_copy.imageSubresource.baseArrayLayer = 0;
			buffer_image_copy.imageSubresource.layerCount = 1;
			buffer_image_copy.imageSubresource.mipLevel = i;
			buffer_image_copy.bufferRowLength = 0;
			buffer_image_copy.bufferImageHeight = 0;

			buffer_offset += buffer_image_copy.imageExtent.width * buffer_image_copy.imageExtent.height * (m_Specification.format == ImageFormat::BC7 ? 1 : 4);
		}

		// Submit copy command
		vkCmdCopyBufferToImage(cmd_buffer, staging_buffer.Raw(), m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copy_regions.size(), copy_regions.data());
		
		// Transition layout to shader read only
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = texture_create_info.mipLevels;

		vkCmdPipelineBarrier(cmd_buffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);

		// Execute commands
		device->ExecuteTransientCmdBuffer(cmd_buffer);

		m_CurrentLayout = ImageLayout::SHADER_READ_ONLY;

		// Clear image data
		m_Specification.pixels.clear();

		// Create image view
		VkImageViewCreateInfo image_view_create_info = {};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.image = m_Image;
		image_view_create_info.format = convert(m_Specification.format);
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

		Ref<DeviceCmdBuffer> cmd_buffer = DeviceCmdBuffer::Create(
			&g_TransientAllocator,
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
			(BitMask)PipelineAccess::NONE,
			(BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE
		);
		cmd_buffer->End();
		cmd_buffer->Execute(true);
		cmd_buffer->Destroy();
	}

	void VulkanImage::CreateDepthBuffer()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VkImageCreateInfo depth_buffer_create_info = {};
		depth_buffer_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		depth_buffer_create_info.extent = { m_Specification.extent.x, m_Specification.extent.y, 1 };
		depth_buffer_create_info.imageType = VK_IMAGE_TYPE_2D;
		depth_buffer_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_buffer_create_info.mipLevels = 1;
		depth_buffer_create_info.arrayLayers = 1;
		depth_buffer_create_info.format = convert(m_Specification.format);
		depth_buffer_create_info.samples = VK_SAMPLE_COUNT_1_BIT; // HACK
		depth_buffer_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depth_buffer_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		depth_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto allocator = VulkanMemoryAllocator::Get();
		m_Allocation = allocator->AllocateImage(&depth_buffer_create_info, 0, &m_Image);

		VkImageViewCreateInfo image_view_create_info = {};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.image = m_Image;
		image_view_create_info.format = convert(m_Specification.format);
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		VK_CHECK_RESULT(vkCreateImageView(device->Raw(), &image_view_create_info, nullptr, &m_ImageView));

		Ref<DeviceCmdBuffer> cmd_buffer = DeviceCmdBuffer::Create(
			&g_TransientAllocator,
			DeviceCmdBufferLevel::PRIMARY,
			DeviceCmdBufferType::TRANSIENT,
			DeviceCmdType::GENERAL
		);

		cmd_buffer->Begin();
		SetLayout(
			cmd_buffer,
			ImageLayout::DEPTH_STENCIL_ATTACHMENT,
			PipelineStage::TOP_OF_PIPE,
			PipelineStage::COLOR_ATTACHMENT_OUTPUT,
			(BitMask)PipelineAccess::NONE,
			(BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE
		);
		cmd_buffer->End();
		cmd_buffer->Execute(true);
		cmd_buffer->Destroy();
	}

	void VulkanImage::CreateStorageImage()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VkImageCreateInfo image_create_info = {};
		image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_create_info.extent = { m_Specification.extent.x, m_Specification.extent.y, 1 };
		image_create_info.imageType = VK_IMAGE_TYPE_2D;
		image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_create_info.mipLevels = 1;
		image_create_info.arrayLayers = 1;
		image_create_info.format = convert(m_Specification.format);
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT; // HACK
		image_create_info.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto allocator = VulkanMemoryAllocator::Get();
		m_Allocation = allocator->AllocateImage(&image_create_info, 0, &m_Image);

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

		Ref<DeviceCmdBuffer> cmd_buffer = DeviceCmdBuffer::Create(
			&g_TransientAllocator,
			DeviceCmdBufferLevel::PRIMARY,
			DeviceCmdBufferType::TRANSIENT,
			DeviceCmdType::GENERAL
		);

		cmd_buffer->Begin();
		SetLayout(
			cmd_buffer,
			ImageLayout::GENERAL,
			PipelineStage::TOP_OF_PIPE,
			PipelineStage::COLOR_ATTACHMENT_OUTPUT,
			(BitMask)PipelineAccess::NONE,
			(BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE
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
		auto sampler = m_Sampler;
		auto device = VulkanGraphicsContext::Get()->GetDevice()->Raw();
		RuntimeExecutionContext::Get().GetObjectLifetimeManager().EnqueueObjectDeletion(
			[sampler, device]() {
				vkDestroySampler(device, sampler, nullptr);
			}
		);
		m_Sampler = VK_NULL_HANDLE;
	}

}