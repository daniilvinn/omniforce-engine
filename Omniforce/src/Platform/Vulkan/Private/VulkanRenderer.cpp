#include "../VulkanRenderer.h"

#include "../VulkanGraphicsContext.h"
#include "../VulkanDevice.h"
#include "../VulkanPipeline.h"
#include "../VulkanDeviceBuffer.h"
#include "../VulkanImage.h"
#include "../VulkanDescriptorSet.h"

#include <Renderer/ShaderLibrary.h>
#include <Threading/JobSystem.h>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

namespace Omni {

	VkDescriptorPool VulkanRenderer::s_DescriptorPool = VK_NULL_HANDLE;

	VulkanRenderer::VulkanRenderer(const RendererConfig& config)
		: m_Config(config)
	{
		m_GraphicsContext = std::make_shared<VulkanGraphicsContext>(config);
		m_Device = m_GraphicsContext->GetDevice();

		auto base_swapchain = m_GraphicsContext->GetSwapchain();
		m_Swapchain = ShareAs<VulkanSwapchain>(base_swapchain);

		m_CmdBuffers.resize(m_Swapchain->GetSpecification().frames_in_flight);
		m_CmdBuffers.shrink_to_fit();

		for (auto& buf : m_CmdBuffers) {
			buf = std::make_shared<VulkanDeviceCmdBuffer>(DeviceCmdBufferLevel::PRIMARY, DeviceCmdBufferType::GENERAL, DeviceCmdType::GENERAL);
		}

		if (s_DescriptorPool == VK_NULL_HANDLE) {
			uint32 descriptor_count = UINT16_MAX * config.frames_in_flight;;

			std::vector<VkDescriptorPoolSize> pool_sizes = {
				{ VK_DESCRIPTOR_TYPE_SAMPLER,					descriptor_count }, 
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	descriptor_count },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,				descriptor_count },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,				descriptor_count },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			descriptor_count },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			descriptor_count },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,			descriptor_count }
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
		for (auto& cmd_buffer : m_CmdBuffers)
			cmd_buffer->Destroy();
		vkDestroyDescriptorPool(m_Device->Raw(), s_DescriptorPool, nullptr);
	}

	void VulkanRenderer::BeginFrame()
	{
		m_Swapchain->BeginFrame();	
		m_CurrentCmdBuffer = m_CmdBuffers[m_Swapchain->GetCurrentFrameIndex()];
		this->BeginCommandRecord();
	}

	void VulkanRenderer::EndFrame()
	{
		m_Swapchain->EndFrame();
	}

	void VulkanRenderer::BeginCommandRecord()
	{
		Renderer::Submit([=]() mutable {
			m_CurrentCmdBuffer->Reset();
			m_CurrentCmdBuffer->Begin();
		});
	}

	void VulkanRenderer::EndCommandRecord()
	{
		Renderer::Submit([=]() mutable {
			m_CurrentCmdBuffer->End();
		});
	}

	void VulkanRenderer::ExecuteCurrentCommands()
	{
		Renderer::Submit(
			[=]() mutable {
				// Execution code
				VkPipelineStageFlags stagemasks[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

				Shared<VulkanDeviceCmdBuffer> vk_cmd_buffer = ShareAs<VulkanDeviceCmdBuffer>(m_CurrentCmdBuffer);
				VkCommandBuffer raw_buffer = vk_cmd_buffer->Raw();
				auto semaphores = m_Swapchain->GetSemaphores();

				VkSubmitInfo submitinfo = {};
				submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitinfo.commandBufferCount = 1;
				submitinfo.pCommandBuffers = &raw_buffer;
				submitinfo.signalSemaphoreCount = 1;
				submitinfo.pSignalSemaphores = &semaphores.render_complete;
				submitinfo.waitSemaphoreCount = 1;
				submitinfo.pWaitSemaphores = &semaphores.present_complete;
				submitinfo.pWaitDstStageMask = stagemasks;

				m_Mutex.lock();
				VkResult result = vkQueueSubmit(m_Device->GetGeneralQueue(), 1, &submitinfo, m_Swapchain->GetCurrentFence());
				m_Mutex.unlock();
		});
	}

	void VulkanRenderer::ClearImage(Shared<Image> image, const fvec4& value)
	{
		Renderer::Submit([=]() mutable {

			image->SetLayout(
				m_CurrentCmdBuffer,
				ImageLayout::TRANSFER_DST,
				PipelineStage::TOP_OF_PIPE,
				PipelineStage::TRANSFER
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

			vkCmdClearColorImage(m_CurrentCmdBuffer->Raw(), vk_image->Raw(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &range);

			image->SetLayout(m_CurrentCmdBuffer,
				ImageLayout::PRESENT_SRC,
				PipelineStage::TRANSFER,
				PipelineStage::BOTTOM_OF_PIPE
			);
		});
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

	uint32 VulkanRenderer::GetDeviceMinimalUniformBufferAlignment() const
	{
		return m_Device->GetPhysicalDevice()->GetProps().limits.minUniformBufferOffsetAlignment;
	}

	uint32 VulkanRenderer::GetDeviceMinimalStorageBufferAlignment() const
	{
		return m_Device->GetPhysicalDevice()->GetProps().limits.minStorageBufferOffsetAlignment;
	}

	void VulkanRenderer::RenderMesh(Shared<Pipeline> pipeline, Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo, MiscData misc_data)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);
			Shared<VulkanDeviceBuffer> vk_vbo = ShareAs<VulkanDeviceBuffer>(vbo);
			Shared<VulkanDeviceBuffer> vk_ibo = ShareAs<VulkanDeviceBuffer>(ibo);

			IndexBufferData* ibo_data = (IndexBufferData*)vk_ibo->GetAdditionalData();

			VkDeviceSize offset = 0;

			VkBuffer raw_vbo = vk_vbo->Raw();

			vkCmdBindPipeline(m_CurrentCmdBuffer->Raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->Raw());
			vkCmdBindVertexBuffers(m_CurrentCmdBuffer->Raw(), 0, 1, &raw_vbo, &offset);
			vkCmdBindIndexBuffer(m_CurrentCmdBuffer->Raw(), vk_ibo->Raw(), 0, ibo_data->index_type);

			if(misc_data.size)
				vkCmdPushConstants(m_CurrentCmdBuffer->Raw(), vk_pipeline->RawLayout(), VK_SHADER_STAGE_ALL, 0, misc_data.size, misc_data.data);

			vkCmdDrawIndexed(m_CurrentCmdBuffer->Raw(), ibo_data->index_count, 1, 0, 0, 0);

			delete[] misc_data.data;
		});
	}

	void VulkanRenderer::RenderQuad(Shared<Pipeline> pipeline, MiscData data)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);

			vkCmdPushConstants(m_CurrentCmdBuffer->Raw(), vk_pipeline->RawLayout(), VK_SHADER_STAGE_ALL, 0, data.size, data.data);
			vkCmdBindPipeline(m_CurrentCmdBuffer->Raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->Raw());

			vkCmdDraw(m_CurrentCmdBuffer->Raw(), 6, 1, 0, 0);

			delete[] data.data;
		});
	}

	void VulkanRenderer::RenderQuad(Shared<Pipeline> pipeline, uint32 amount, MiscData data)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);

			if (data.size)
			{
				vkCmdPushConstants(m_CurrentCmdBuffer->Raw(), vk_pipeline->RawLayout(), VK_SHADER_STAGE_ALL, 0, data.size, data.data);
				delete[] data.data;
			}
			vkCmdBindPipeline(m_CurrentCmdBuffer->Raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->Raw());
			vkCmdDraw(m_CurrentCmdBuffer->Raw(), 6, amount, 0, 0);
		});
	}

	void VulkanRenderer::RenderLines(Shared<Pipeline> pipeline, uint32 amount, MiscData data)
	{
		Renderer::Submit([=]()mutable {
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);

			if (data.size)
			{
				vkCmdPushConstants(m_CurrentCmdBuffer->Raw(), vk_pipeline->RawLayout(), VK_SHADER_STAGE_ALL, 0, data.size, data.data);
				delete[] data.data;
			}
			vkCmdBindPipeline(m_CurrentCmdBuffer->Raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->Raw());
			vkCmdDraw(m_CurrentCmdBuffer->Raw(), 6, amount, 0, 0);
		});
	}

	void VulkanRenderer::RenderImGui()
	{
		auto image = m_Swapchain->GetCurrentImage();
		
		Renderer::Submit([=]() mutable {
			image->SetLayout(
				m_CurrentCmdBuffer,
				ImageLayout::COLOR_ATTACHMENT,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				PipelineStage::BOTTOM_OF_PIPE,
				PipelineAccess::COLOR_ATTACHMENT_WRITE,
				PipelineAccess::NONE
			);
		});

		BeginRender(
			image,
			image->GetSpecification().extent,
			{ 0,0 },
			{ 0.0f, 0.0f, 0.0f, 0.0f }
		);

		Renderer::Submit([=]() mutable {
			ImGui::Render();
			ImDrawData* draw_data = ImGui::GetDrawData();
			ImGui_ImplVulkan_RenderDrawData(draw_data, m_CurrentCmdBuffer->Raw());
		});
		EndRender(image);
	}

	void VulkanRenderer::CopyToSwapchain(Shared<Image> image)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanImage> vk_image = ShareAs<VulkanImage>(image);
			Shared<Image> swapchain_image = m_Swapchain->GetCurrentImage();
			uvec3 swapchain_resolution = swapchain_image->GetSpecification().extent;
			uvec3 src_image_resolution = image->GetSpecification().extent;

			VkImageBlit image_blit = {};
			image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_blit.srcSubresource.mipLevel = 0;
			image_blit.srcSubresource.baseArrayLayer = 0;
			image_blit.srcSubresource.layerCount = 1;
			image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_blit.dstSubresource.mipLevel = 0;
			image_blit.dstSubresource.baseArrayLayer = 0;
			image_blit.dstSubresource.layerCount = 1;
			image_blit.srcOffsets[0] = { 0, 0, 0 };
			image_blit.srcOffsets[1] = { (int32)src_image_resolution.x, (int32)src_image_resolution.y, 1 };
			image_blit.dstOffsets[0] = { 0, 0, 0 };
			image_blit.dstOffsets[1] = { (int32)swapchain_resolution.x, (int32)swapchain_resolution.y, 1 };

			vk_image->SetLayout(
				m_CurrentCmdBuffer,
				ImageLayout::TRANSFER_DST,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				PipelineStage::TRANSFER,
				PipelineAccess::COLOR_ATTACHMENT_WRITE,
				PipelineAccess::TRANSFER_READ
			);

			vkCmdBlitImage(
				m_CurrentCmdBuffer->Raw(),
				vk_image->Raw(),
				(VkImageLayout)vk_image->GetCurrentLayout(),
				ShareAs<VulkanImage>(image)->Raw(),
				(VkImageLayout)VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&image_blit,
				VK_FILTER_LINEAR
			);
		});
	}

	void VulkanRenderer::BeginRender(Shared<Image> target, uvec3 render_area, ivec2 render_offset, fvec4 clear_color)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanImage> vk_target = ShareAs<VulkanImage>(target);
			ImageSpecification target_spec = vk_target->GetSpecification();

			target->SetLayout(m_CurrentCmdBuffer,
				ImageLayout::COLOR_ATTACHMENT,
				PipelineStage::BOTTOM_OF_PIPE,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				PipelineAccess::NONE,
				PipelineAccess::COLOR_ATTACHMENT_WRITE
			);

			VkRenderingAttachmentInfo color_attachment = {};
			color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			color_attachment.imageView = vk_target->RawView();
			color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			if(clear_color.a != 0.0f)
				color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			else
				color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment.clearValue = { clear_color.r, clear_color.g, clear_color.b, clear_color.a };

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
			vkCmdSetScissor(m_CurrentCmdBuffer->Raw(), 0, 1, &scissor);
			vkCmdSetViewport(m_CurrentCmdBuffer->Raw(), 0, 1, &viewport);
			vkCmdBeginRendering(m_CurrentCmdBuffer->Raw(), &rendering_info);
		});
	}

	void VulkanRenderer::EndRender(Shared<Image> target)
	{
		Renderer::Submit([=]() mutable {
			vkCmdEndRendering(m_CurrentCmdBuffer->Raw());
		});
	}

	void VulkanRenderer::WaitDevice()
	{
		vkDeviceWaitIdle(m_Device->Raw());
	}

	void VulkanRenderer::BindSet(Shared<DescriptorSet> set, Shared<Pipeline> pipeline, uint8 index)
	{
		Renderer::Submit([=]() mutable {
			PipelineSpecification pipeline_spec = pipeline->GetSpecification();
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);
			Shared<VulkanDescriptorSet> vk_set = ShareAs<VulkanDescriptorSet>(set);
			VkDescriptorSet raw_set = vk_set->Raw();

			VkPipelineBindPoint bind_point;

			switch (pipeline_spec.type)
			{
			case PipelineType::GRAPHICS:		bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;			break;
			case PipelineType::COMPUTE:			bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;			break;
			case PipelineType::RAY_TRACING:		bind_point = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;	break;
			default:							std::unreachable();
			}

			vkCmdBindDescriptorSets(m_CurrentCmdBuffer->Raw(), bind_point, vk_pipeline->RawLayout(), index, 1, &raw_set, 0, nullptr);
		});
	}

}