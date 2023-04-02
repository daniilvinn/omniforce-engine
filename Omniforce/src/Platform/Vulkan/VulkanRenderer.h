#pragma once

#include <Renderer/RendererAPI.h>
#include "VulkanCommon.h"

#include <Renderer/Swapchain.h>
#include <Platform/Vulkan/VulkanSwapchain.h>

#include <Renderer/Renderer.h>

namespace Omni {

	class VulkanRenderer : public RendererAPI {
	public:
		struct CmdBuffer {
			VkCommandBuffer buffer;
			VkCommandPool pool;
		};

		VulkanRenderer(const RendererConfig& config);
		~VulkanRenderer() override;

		RendererConfig GetConfig() const override { return m_Config; }
		uint32 GetCurrentFrameIndex() const override { return m_Swapchain->GetCurrentFrameIndex(); }

		void BeginFrame() override;
		void EndFrame() override;
		void BeginRender(Shared<Image> target, uvec3 render_area, ivec2 render_offset, fvec4 clear_color) override;
		void EndRender(Shared<Image> target) override;
		void WaitDevice() override;

		void BeginCommandRecord() override;
		void EndCommandRecord() override;
		void ExecuteCurrentCommands() override;

		void ClearImage(Shared<Image> image, const fvec4& value) override;
		Shared<Swapchain> GetSwapchain() override { return ShareAs<Swapchain>(m_Swapchain); };
		CmdBuffer* GetCurrentCmdBuffer() const { return m_CurrentCmdBuffer; }
		void RenderMesh(Shared<Pipeline> pipeline, Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo) override;

		void TransitionImageLayout(
			Shared<VulkanImage> image, 
			VkImageLayout new_layout, 
			VkPipelineStageFlags srcStage, 
			VkPipelineStageFlags dstStage, 
			VkAccessFlags srcAccess, 
			VkAccessFlags dstAccess
		);

		static std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout layout, uint32 count);
		static void FreeDescriptorSets(std::vector<VkDescriptorSet> sets);

	private:
		RendererConfig m_Config;

		Shared<VulkanGraphicsContext> m_GraphicsContext;
		Shared<VulkanDevice> m_Device;
		Shared<VulkanSwapchain> m_Swapchain;

		std::vector<CmdBuffer> m_CmdBuffers;
		CmdBuffer* m_CurrentCmdBuffer;

		std::shared_mutex m_Mutex;

		static VkDescriptorPool s_DescriptorPool;
	};

}
