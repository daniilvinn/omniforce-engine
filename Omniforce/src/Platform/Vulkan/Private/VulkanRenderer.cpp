#include "../VulkanRenderer.h"

#include "../VulkanGraphicsContext.h"
#include "../VulkanDevice.h"
#include "../VulkanPipeline.h"
#include "../VulkanDeviceBuffer.h"
#include "../VulkanImage.h"
#include <Renderer/ShaderLibrary.h>
#include <Threading/JobSystem.h>

namespace Omni {

	VkDescriptorPool VulkanRenderer::s_DescriptorPool = VK_NULL_HANDLE;

	VulkanRenderer::VulkanRenderer(const RendererConfig& config)
	{
		m_GraphicsContext = std::make_shared<VulkanGraphicsContext>(config);
		m_Device = m_GraphicsContext->GetDevice();

		auto base_swapchain = m_GraphicsContext->GetSwapchain();
		m_Swapchain = ShareAs<VulkanSwapchain>(base_swapchain);

		m_CmdBuffers.resize(m_Swapchain->GetSpecification().frames_in_flight);
		m_CmdBuffers.shrink_to_fit();

		for (auto& cmd_buffer : m_CmdBuffers) 
		{
			VkCommandPoolCreateInfo cmd_pool_create_info = {};
			cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmd_pool_create_info.queueFamilyIndex = m_Device->GetPhysicalDevice()->GetQueueFamilyIndices().graphics;

			VK_CHECK_RESULT(vkCreateCommandPool(m_Device->Raw(), &cmd_pool_create_info, nullptr, &cmd_buffer.pool));

			VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {};
			cmd_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmd_buffer_allocate_info.commandPool = cmd_buffer.pool;
			cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmd_buffer_allocate_info.commandBufferCount = 1;
			
			VK_CHECK_RESULT(vkAllocateCommandBuffers(m_Device->Raw(), &cmd_buffer_allocate_info, &cmd_buffer.buffer));
		}

		if (s_DescriptorPool == VK_NULL_HANDLE) {
			std::vector<VkDescriptorPoolSize> pool_sizes = {
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
				{ VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK, 1000 }
				// { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1000 },
				// { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 1000 },
				// { VK_DESCRIPTOR_TYPE_MUTABLE_VALVE, 1000 }
			};

			VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
			descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptor_pool_create_info.maxSets = 1000;
			descriptor_pool_create_info.poolSizeCount = pool_sizes.size();
			descriptor_pool_create_info.pPoolSizes = pool_sizes.data();
			descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			vkCreateDescriptorPool(m_Device->Raw(), &descriptor_pool_create_info, nullptr, &s_DescriptorPool);
		}
	}

	VulkanRenderer::~VulkanRenderer()
	{
		vkDeviceWaitIdle(m_Device->Raw());
		for (auto& cmd_buffer : m_CmdBuffers) {
			vkFreeCommandBuffers(m_Device->Raw(), cmd_buffer.pool, 1, &cmd_buffer.buffer);
			vkDestroyCommandPool(m_Device->Raw(), cmd_buffer.pool, nullptr);
		}
		vkDestroyDescriptorPool(m_Device->Raw(), s_DescriptorPool, nullptr);
	}

	void VulkanRenderer::BeginFrame()
	{
		m_Swapchain->BeginFrame();	
		m_CurrentCmdBuffer = &m_CmdBuffers[m_Swapchain->GetCurrentFrameIndex()];
		this->BeginCommandRecord();
	}

	void VulkanRenderer::EndFrame()
	{
		m_Swapchain->EndFrame();
	}

	void VulkanRenderer::BeginCommandRecord()
	{
		Renderer::Submit([=]() mutable {
			vkResetCommandPool(m_Device->Raw(), m_CurrentCmdBuffer->pool, 0);

			VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
			cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			vkBeginCommandBuffer(m_CurrentCmdBuffer->buffer, &cmd_buffer_begin_info);
		});
	}

	void VulkanRenderer::EndCommandRecord()
	{
		Renderer::Submit([cmd_buffer = m_CurrentCmdBuffer]() {
			vkEndCommandBuffer(cmd_buffer->buffer);
		});
	}

	void VulkanRenderer::ExecuteCurrentCommands()
	{
		Renderer::Submit(
			[
				// Context
				current_fence = m_Swapchain->GetCurrentFence(),
				current_render_semaphore = m_Swapchain->GetSemaphores().render_complete,
				current_present_semaphore = m_Swapchain->GetSemaphores().present_complete,
				cmd_buffer = m_CurrentCmdBuffer,
				device = m_Device,
				mtx = &m_Mutex
			]() {
				// Execution code
				VkPipelineStageFlags stagemasks[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

				VkSubmitInfo submitinfo = {};
				submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitinfo.commandBufferCount = 1;
				submitinfo.pCommandBuffers = &cmd_buffer->buffer;
				submitinfo.signalSemaphoreCount = 1;
				submitinfo.pSignalSemaphores = &current_render_semaphore;
				submitinfo.waitSemaphoreCount = 1;
				submitinfo.pWaitSemaphores = &current_present_semaphore;
				submitinfo.pWaitDstStageMask = stagemasks;

				mtx->lock();
				VkResult result = vkQueueSubmit(device->GetGeneralQueue(), 1, &submitinfo, current_fence);
				mtx->unlock();
		});
	}

	void VulkanRenderer::ClearImage(Shared<Image> image, const fvec4& value)
	{
		Renderer::Submit([=]() mutable {
			TransitionImageLayout(
				ShareAs<VulkanImage>(image),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0
			);
			Shared<VulkanImage> vk_image = ShareAs<VulkanImage>(image);

			VkClearColorValue clear_color_value = {};
			clear_color_value.float32[0] = value.r;
			clear_color_value.float32[1] = value.g;
			clear_color_value.float32[2] = value.b;
			clear_color_value.float32[3] = value.a;

			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			vkCmdClearColorImage(m_CurrentCmdBuffer->buffer, vk_image->Raw(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &range);

			TransitionImageLayout(
				ShareAs<VulkanImage>(image),
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
				0
			);
		});
	}

	void VulkanRenderer::TransitionImageLayout(Shared<VulkanImage> image, VkImageLayout new_layout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess)
	{
		VkImageMemoryBarrier image_memory_barrier = {};
		image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_memory_barrier.image = image->Raw();
		image_memory_barrier.oldLayout = image->GetCurrentLayout();
		image_memory_barrier.newLayout = new_layout;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.srcAccessMask = srcAccess;
		image_memory_barrier.dstAccessMask = dstAccess;
		image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(m_CurrentCmdBuffer->buffer,
			srcStage,
			dstStage,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&image_memory_barrier
		);
		
		image->SetCurrentLayout(new_layout);
	}

	std::vector<VkDescriptorSet> VulkanRenderer::AllocateDescriptorSets(VkDescriptorSetLayout layout, uint32 count)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		std::vector<VkDescriptorSet> sets(count);
		sets.resize(count);

		VkDescriptorSetAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocate_info.descriptorPool = s_DescriptorPool;
		allocate_info.descriptorSetCount = count;
		allocate_info.pSetLayouts = &layout;

		if (vkAllocateDescriptorSets(device->Raw(), &allocate_info, sets.data()) != VK_SUCCESS) 
		{
			OMNIFORCE_CORE_ERROR("Failed to allocate descriptor set. Possible issue: too many allocated descriptor sets.");
			return std::vector<VkDescriptorSet>(0);
		};

		return sets;
	}

	void VulkanRenderer::FreeDescriptorSets(std::vector<VkDescriptorSet> sets)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VK_CHECK_RESULT(vkFreeDescriptorSets(device->Raw(), s_DescriptorPool, sets.size(), sets.data()));
	}

	void VulkanRenderer::RenderMesh(Shared<Pipeline> pipeline, Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);
			Shared<VulkanDeviceBuffer> vk_vbo = ShareAs<VulkanDeviceBuffer>(vbo);
			Shared<VulkanDeviceBuffer> vk_ibo = ShareAs<VulkanDeviceBuffer>(ibo);

			IndexBufferData* ibo_data = (IndexBufferData*)vk_ibo->GetAdditionalData();

			VkDeviceSize offset = 0;

			VkBuffer raw_vbo = vk_vbo->Raw();

			vkCmdBindPipeline(m_CurrentCmdBuffer->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->Raw());
			vkCmdBindVertexBuffers(m_CurrentCmdBuffer->buffer, 0, 1, &raw_vbo, &offset);
			vkCmdBindIndexBuffer(m_CurrentCmdBuffer->buffer, vk_ibo->Raw(), 0, ibo_data->index_type);

			vkCmdDrawIndexed(m_CurrentCmdBuffer->buffer, ibo_data->index_count, 1, 0, 0, 0);
		});
	}

	void VulkanRenderer::BeginRender(Shared<Image> target, uvec2 render_area, ivec2 render_offset, fvec4 clear_color)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanImage> vk_target = ShareAs<VulkanImage>(target);
			ImageSpecification target_spec = vk_target->GetSpecification();

			TransitionImageLayout(ShareAs<VulkanImage>(target),
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
			);


			VkRenderingAttachmentInfo color_attachment = {};
			color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			color_attachment.imageView = vk_target->RawView();
			color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			VkRenderingInfo rendering_info = {};
			rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			rendering_info.renderArea = { { render_offset.x, render_offset.y }, { render_area.x, render_area.y } };
			rendering_info.layerCount = 1;
			rendering_info.colorAttachmentCount = 1;
			rendering_info.pColorAttachments = &color_attachment;
			rendering_info.pDepthAttachment = nullptr;
			rendering_info.pStencilAttachment = nullptr;

			VkRect2D scissor = { {0,0}, {target_spec.extent.x, target_spec.extent.y} };
			VkViewport viewport = { 0, (float32)target_spec.extent.y, (float32)target_spec.extent.x, -(float32)target_spec.extent.y, 0.0f, 1.0f};
			vkCmdSetScissor(m_CurrentCmdBuffer->buffer, 0, 1, &scissor);
			vkCmdSetViewport(m_CurrentCmdBuffer->buffer, 0, 1, &viewport);
			vkCmdBeginRendering(m_CurrentCmdBuffer->buffer, &rendering_info);
		});
	}

	void VulkanRenderer::EndRender(Shared<Image> target)
	{
		Renderer::Submit([=]() mutable {
			vkCmdEndRendering(m_CurrentCmdBuffer->buffer);
			TransitionImageLayout(ShareAs<VulkanImage>(target),
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				0
			);
		});
	}

	void VulkanRenderer::WaitDevice()
	{
		vkDeviceWaitIdle(m_Device->Raw());
	}

}