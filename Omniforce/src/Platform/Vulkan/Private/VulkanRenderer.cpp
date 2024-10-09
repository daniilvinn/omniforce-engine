#include "../VulkanRenderer.h"

#include "../VulkanGraphicsContext.h"
#include "../VulkanDevice.h"
#include "../VulkanPipeline.h"
#include "../VulkanDeviceBuffer.h"
#include "../VulkanImage.h"
#include "../VulkanDescriptorSet.h"

#include <Renderer/PipelineBarrier.h>
#include <Renderer/ShaderLibrary.h>
#include <Threading/JobSystem.h>

#include <imgui.h>

#include "backends/imgui_impl_vulkan.h"

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
			bool is_storage_image = image->GetSpecification().usage == ImageUsage::STORAGE_IMAGE;

			if (!is_storage_image) {
				image->SetLayout(
					m_CurrentCmdBuffer,
					ImageLayout::TRANSFER_DST,
					PipelineStage::TOP_OF_PIPE,
					PipelineStage::TRANSFER
				);
			}


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

			vkCmdClearColorImage(m_CurrentCmdBuffer->Raw(), vk_image->Raw(), is_storage_image ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &range);

			if (!is_storage_image) {
				image->SetLayout(m_CurrentCmdBuffer,
					ImageLayout::PRESENT_SRC,
					PipelineStage::TRANSFER,
					PipelineStage::BOTTOM_OF_PIPE
				);
			}
		});
	}

	void VulkanRenderer::RenderMeshTasks(Shared<Pipeline> pipeline, const glm::uvec3& dimensions, MiscData data)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);

			if (data.size) {
				vkCmdPushConstants(m_CurrentCmdBuffer->Raw(), vk_pipeline->RawLayout(), VK_SHADER_STAGE_ALL, 0, data.size, data.data);
				delete[] data.data;
			}
			vkCmdBindPipeline(m_CurrentCmdBuffer->Raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->Raw());
			vkCmdDrawMeshTasksEXT(m_CurrentCmdBuffer->Raw(), dimensions.x, dimensions.y, dimensions.z);
		});
	}

	void VulkanRenderer::RenderMeshTasksIndirect(Shared<Pipeline> pipeline, Shared<DeviceBuffer> params, MiscData data)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);
			Shared<VulkanDeviceBuffer> vk_buffer = ShareAs<VulkanDeviceBuffer>(params);

			vkCmdBindPipeline(m_CurrentCmdBuffer->Raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->Raw());
			if (data.size) {
				vkCmdPushConstants(m_CurrentCmdBuffer->Raw(), vk_pipeline->RawLayout(), VK_SHADER_STAGE_ALL, 0, data.size, data.data);
				delete[] data.data;
			}
			uint32 max_draws = (vk_buffer->GetSpecification().size - 4) / sizeof glm::uvec3;
			vkCmdDrawMeshTasksIndirectCountEXT(m_CurrentCmdBuffer->Raw(), vk_buffer->Raw(), 4, vk_buffer->Raw(), 0, max_draws, sizeof VkDrawMeshTasksIndirectCommandEXT);
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
		return m_Device->GetPhysicalDevice()->GetProperties().properties.limits.minUniformBufferOffsetAlignment;
	}

	uint32 VulkanRenderer::GetDeviceMinimalStorageBufferAlignment() const
	{
		return m_Device->GetPhysicalDevice()->GetProperties().properties.limits.minStorageBufferOffsetAlignment;
	}

	uint32 VulkanRenderer::GetDeviceOptimalTaskWorkGroupSize() const
	{
		void* node = m_Device->GetPhysicalDevice()->GetProperties().pNext;
		VkStructureType type = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
		while (node != nullptr) {
			if (memcmp(node, &type, sizeof VkStructureType) == 0) {
				// If allowed, use smaller work group size
				return std::min(((VkPhysicalDeviceMeshShaderPropertiesEXT*)node)->maxPreferredTaskWorkGroupInvocations, 128u);
			}

			intptr_t offset_ptr = ((intptr_t)node + 4); // offsetting to pNext
			node = (void*)offset_ptr;
		}

		OMNIFORCE_ASSERT_TAGGED(false, "Failed to find VkPhysicalDeviceMeshShaderPropertiesEXT structure");
		return 0;
	}

	uint32 VulkanRenderer::GetDeviceOptimalMeshWorkGroupSize() const
	{
		void* node = m_Device->GetPhysicalDevice()->GetProperties().pNext;
		VkStructureType type = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
		while (node != nullptr) {
			if (memcmp(node, &type, sizeof VkStructureType) == 0) {
				// If allowed, use smaller work group size
				return std::min(((VkPhysicalDeviceMeshShaderPropertiesEXT*)node)->maxPreferredMeshWorkGroupInvocations, 64u);
			}

			intptr_t offset_ptr = ((intptr_t)node + 4); // offsetting to pNext
			node = (void*)offset_ptr;
		}

		OMNIFORCE_ASSERT_TAGGED(false, "Failed to find VkPhysicalDeviceMeshShaderPropertiesEXT structure");
		return 0;
	}

	uint32 VulkanRenderer::GetDeviceOptimalComputeWorkGroupSize() const
	{
		return 0;
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

	void VulkanRenderer::DispatchCompute(Shared<Pipeline> pipeline, const glm::uvec3& dimensions, MiscData data)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);
			
			if (data.size) {
				vkCmdPushConstants(m_CurrentCmdBuffer->Raw(), vk_pipeline->RawLayout(), VK_SHADER_STAGE_ALL, 0, data.size, data.data);
				delete[] data.data;
			}
			vkCmdBindPipeline(m_CurrentCmdBuffer->Raw(), VK_PIPELINE_BIND_POINT_COMPUTE, vk_pipeline->Raw());
			vkCmdDispatch(m_CurrentCmdBuffer->Raw(), dimensions.x, dimensions.y, dimensions.z);
		});
	}

	void VulkanRenderer::RenderUnindexed(Shared<Pipeline> pipeline, Shared<DeviceBuffer> vertex_buffer, MiscData data)
	{
		Renderer::Submit([=]() mutable {
			Shared<VulkanPipeline> vk_pipeline = ShareAs<VulkanPipeline>(pipeline);

			if (data.size) {
				vkCmdPushConstants(m_CurrentCmdBuffer->Raw(), vk_pipeline->RawLayout(), VK_SHADER_STAGE_ALL, 0, data.size, data.data);
				delete[] data.data;
			}

			Shared<VulkanDeviceBuffer> vk_buffer = ShareAs<VulkanDeviceBuffer>(vertex_buffer);
			VkBuffer vb = vk_buffer->Raw();
			VkDeviceSize offset = 0;

			uint32 vertex_count = ((VertexBufferData*)vk_buffer->GetAdditionalData())->vertex_count;

			vkCmdBindVertexBuffers(m_CurrentCmdBuffer->Raw(), 0, 1, &vb, &offset);

			vkCmdBindPipeline(m_CurrentCmdBuffer->Raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->Raw());
			vkCmdDraw(m_CurrentCmdBuffer->Raw(), vertex_count, 1, 0, 0);
		});
	}

	void VulkanRenderer::RenderImGui()
	{
		auto image = m_Swapchain->GetCurrentImage();

		BeginRender(
			{ image },
			image->GetSpecification().extent,
			{ 0,0 },
			{ 0.0f, 0.0f, 0.0f, 1.0f }
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
				(BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE,
				(BitMask)PipelineAccess::TRANSFER_READ
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

	void VulkanRenderer::InsertBarrier(const PipelineBarrierInfo& barrier)
	{
		Renderer::Submit([=]()mutable {
			std::vector<VkMemoryBarrier2> memory_barriers;
			std::vector<VkImageMemoryBarrier2> image_barriers;

			VkDependencyInfo dependency = {};
			dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

			for (auto& buffer_barrier : barrier.buffer_barriers) {

				VkMemoryBarrier2 vk_barrier = {};
				vk_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
				vk_barrier.srcStageMask = (BitMask)buffer_barrier.second.src_stages;
				vk_barrier.dstStageMask = (BitMask)buffer_barrier.second.dst_stages;
				vk_barrier.srcAccessMask = buffer_barrier.second.src_access_mask;
				vk_barrier.dstAccessMask = buffer_barrier.second.dst_access_mask;

				memory_barriers.push_back(vk_barrier);
			}

			for (auto& image_barrier : barrier.image_barriers) {
				Shared<Image> image = image_barrier.first;
				Shared<VulkanImage> vk_image = ShareAs<VulkanImage>(image);

				VkImageMemoryBarrier2 vk_barrier = {};
				vk_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				vk_barrier.srcStageMask = (BitMask)image_barrier.second.src_stages;
				vk_barrier.dstStageMask = (BitMask)image_barrier.second.dst_stages;
				vk_barrier.srcAccessMask = image_barrier.second.src_access_mask;
				vk_barrier.dstAccessMask = image_barrier.second.dst_access_mask;
				vk_barrier.image = vk_image->Raw();
				vk_barrier.oldLayout = (VkImageLayout)vk_image->GetCurrentLayout();
				vk_barrier.newLayout = (VkImageLayout)image_barrier.second.new_image_layout;
				vk_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				vk_barrier.subresourceRange.baseArrayLayer = 0;
				vk_barrier.subresourceRange.layerCount = image->GetSpecification().array_layers;
				vk_barrier.subresourceRange.baseMipLevel = 0;
				vk_barrier.subresourceRange.levelCount = image->GetSpecification().mip_levels;

				vk_image->SetCurrentLayout(image_barrier.second.new_image_layout);

				image_barriers.push_back(vk_barrier);
			}	

			dependency.memoryBarrierCount = memory_barriers.size();
			dependency.pMemoryBarriers = memory_barriers.data();
			dependency.imageMemoryBarrierCount = image_barriers.size();
			dependency.pImageMemoryBarriers = image_barriers.data();

			vkCmdPipelineBarrier2(
				m_CurrentCmdBuffer->Raw(),
				&dependency
			);
		});
	}

	void VulkanRenderer::BeginRender(const std::vector<Shared<Image>> attachments, uvec3 render_area, ivec2 render_offset, fvec4 clear_color)
	{
		Renderer::Submit([=]() mutable {

			VkRenderingAttachmentInfo depth_attachment = {};

			std::vector<VkRenderingAttachmentInfo> color_attachments = {};

			for (auto attachment : attachments) {
				Shared<VulkanImage> vk_target = ShareAs<VulkanImage>(attachment);
				ImageSpecification target_spec = vk_target->GetSpecification();

				if (target_spec.usage == ImageUsage::RENDER_TARGET) {

					VkImageMemoryBarrier target_barrier = {};
					target_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					target_barrier.image = vk_target->Raw();
					target_barrier.oldLayout = (VkImageLayout)vk_target->GetCurrentLayout();
					target_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					target_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
					target_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					target_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					target_barrier.subresourceRange.baseArrayLayer = 0;
					target_barrier.subresourceRange.baseMipLevel = 0;
					target_barrier.subresourceRange.layerCount = 1;
					target_barrier.subresourceRange.levelCount = 1;

					vkCmdPipelineBarrier(m_CurrentCmdBuffer->Raw(),
						VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						0,
						0,
						nullptr,
						0,
						nullptr,
						1, 
						&target_barrier
					);

					vk_target->SetCurrentLayout(ImageLayout::COLOR_ATTACHMENT);

					VkRenderingAttachmentInfo color_attachment = {};
					color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
					color_attachment.imageView = vk_target->RawView();
					color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					if (clear_color.a != 0.0f)
						color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					else
						color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
					color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					color_attachment.clearValue = { clear_color.r, clear_color.g, clear_color.b, clear_color.a };

					color_attachments.push_back(color_attachment);
				}
				else {
					depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
					depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
					depth_attachment.imageView = vk_target->RawView();
					depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					depth_attachment.clearValue.color = { 0,0,0,1 };
					if (clear_color.a != 0.0f)
						depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					else
						depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
					depth_attachment.clearValue.depthStencil = { 0.0f, 0 };
				}
			}
			

			VkRenderingInfo rendering_info = {};
			rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			rendering_info.renderArea = { { render_offset.x, render_offset.y }, { render_area.x, render_area.y } };
			rendering_info.layerCount = 1;
			rendering_info.colorAttachmentCount = color_attachments.size();
			rendering_info.pColorAttachments = color_attachments.data();
			rendering_info.pDepthAttachment = depth_attachment.imageView ? &depth_attachment : nullptr;
			rendering_info.pStencilAttachment = nullptr;



			VkRect2D scissor = { {0,0}, {render_area.x, render_area.y} };
			VkViewport viewport = { 0, (float32)render_area.y, (float32)render_area.x, -(float32)render_area.y, 0.0f, 1.0f};
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