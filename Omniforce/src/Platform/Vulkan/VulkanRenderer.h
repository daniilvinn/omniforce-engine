#pragma once

#include <Renderer/RendererAPI.h>
#include "VulkanCommon.h"

#include <Renderer/Swapchain.h>
#include <Platform/Vulkan/VulkanSwapchain.h>

namespace Omni {

	class VulkanRenderer : public RendererAPI {
	public:
		struct CmdBuffer {
			VkCommandBuffer buffer;
			VkCommandPool pool;
		};

		VulkanRenderer(const RendererConfig& config);
		~VulkanRenderer() override;

		void BeginFrame() override;
		void EndFrame() override;

		void BeginCommandRecord() override;
		void EndCommandRecord() override;

		void ExecuteCurrentCommands() override;

		void ClearImage(Shared<Image> image, const fvec4& value) override;

		Shared<Swapchain> GetSwapchain() override { return ShareAs<Swapchain>(m_Swapchain); };
		CmdBuffer* GetCurrentCmdBuffer() const { return m_CurrentCmdBuffer; }

		void TransitionImageLayout(
			Shared<VulkanImage> image, 
			VkImageLayout new_layout, 
			VkPipelineStageFlags srcStage, 
			VkPipelineStageFlags dstStage, 
			VkAccessFlags srcAccess, 
			VkAccessFlags dstAccess
		);

	private:
		Shared<VulkanGraphicsContext> m_GraphicsContext;
		Shared<VulkanDevice> m_Device;
		Shared<VulkanSwapchain> m_Swapchain;

		std::vector<CmdBuffer> m_CmdBuffers;
		CmdBuffer* m_CurrentCmdBuffer;

		std::shared_mutex m_Mutex;
	};

}
