#pragma once

#include <Renderer/RendererAPI.h>
#include "VulkanCommon.h"

#include <Renderer/Swapchain.h>
#include <Platform/Vulkan/VulkanSwapchain.h>
#include "VulkanDeviceCmdBuffer.h"

#include <Renderer/Renderer.h>

namespace Omni {

	class VulkanRenderer : public RendererAPI {
	public:

		VulkanRenderer(const RendererConfig& config);
		~VulkanRenderer() override;

		RendererConfig GetConfig() const override { return m_Config; }
		uint32 GetCurrentFrameIndex() const override { return m_Swapchain->GetCurrentFrameIndex(); }
		uint32 GetDeviceMinimalUniformBufferAlignment() const override;
		uint32 GetDeviceMinimalStorageBufferAlignment() const override;
		Shared<DeviceCmdBuffer> GetCmdBuffer() override { return m_CurrentCmdBuffer; };
		Shared<Swapchain> GetSwapchain() override { return m_Swapchain; };

		void BeginFrame() override;
		void EndFrame() override;
		void BeginRender(Shared<Image> target, uvec3 render_area, ivec2 render_offset, fvec4 clear_color) override;
		void EndRender(Shared<Image> target) override;
		void WaitDevice() override;
		void BindSet(Shared<DescriptorSet> set, Shared<Pipeline> pipeline, uint8 index) override;
		void CopyToSwapchain(Shared<Image> image) override;

		void ClearImage(Shared<Image> image, const fvec4& value) override;
		void RenderMeshTasks(Shared<Pipeline> pipeline, const glm::vec3& dimensions, MiscData data) override;
		void RenderQuad(Shared<Pipeline> pipeline, MiscData data) override;
		void RenderQuad(Shared<Pipeline> pipeline, uint32 amount, MiscData data) override;
		void RenderImGui() override;

		void BeginCommandRecord() override;
		void EndCommandRecord() override;
		void ExecuteCurrentCommands() override;
		static std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout layout, uint32 count);
		static void FreeDescriptorSets(std::vector<VkDescriptorSet> sets);

	private:
		RendererConfig m_Config;

		Shared<VulkanGraphicsContext> m_GraphicsContext;
		Shared<VulkanDevice> m_Device;
		Shared<VulkanSwapchain> m_Swapchain;

		std::vector<Shared<VulkanDeviceCmdBuffer>> m_CmdBuffers;
		Shared<VulkanDeviceCmdBuffer> m_CurrentCmdBuffer;

		static VkDescriptorPool s_DescriptorPool;

		std::shared_mutex m_Mutex;
	};

}
