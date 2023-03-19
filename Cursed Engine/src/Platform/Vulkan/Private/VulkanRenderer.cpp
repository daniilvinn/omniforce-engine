#include "../VulkanRenderer.h"

#include "../VulkanGraphicsContext.h"
#include "../VulkanDevice.h"

namespace Cursed {

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
	}

	VulkanRenderer::~VulkanRenderer()
	{
		vkDeviceWaitIdle(m_Device->Raw());
		for (auto& cmd_buffer : m_CmdBuffers) {
			vkFreeCommandBuffers(m_Device->Raw(), cmd_buffer.pool, 1, &cmd_buffer.buffer);
			vkDestroyCommandPool(m_Device->Raw(), cmd_buffer.pool, nullptr);
		}
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
		image_memory_barrier.dstAccessMask = srcAccess;
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

}