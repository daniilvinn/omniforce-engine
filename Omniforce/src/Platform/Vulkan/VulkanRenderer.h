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

		void BeginFrame() override;
		void EndFrame() override;
		void BeginRender(Shared<Image> target, uvec3 render_area, ivec2 render_offset, fvec4 clear_color) override;
		void EndRender(Shared<Image> target) override;
		void WaitDevice() override;
		void BindSet(Shared<DescriptorSet> set, Shared<Pipeline> pipeline, uint8 index) override;

		void BeginCommandRecord() override;
		void EndCommandRecord() override;
		void ExecuteCurrentCommands() override;

		void ClearImage(Shared<Image> image, const fvec4& value) override;
		Shared<Swapchain> GetSwapchain() override { return ShareAs<Swapchain>(m_Swapchain); };
		Shared<DeviceCmdBuffer> GetCurrentCmdBuffer() { return ShareAs<DeviceCmdBuffer>(m_CurrentCmdBuffer); }
		void RenderMesh(Shared<Pipeline> pipeline, Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo, MiscData misc_data) override;
		void RenderQuad(Shared<Pipeline> pipeline, MiscData data) override;

		static std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout layout, uint32 count);
		static void FreeDescriptorSets(std::vector<VkDescriptorSet> sets);

		void RenderQuad(Shared<Pipeline> pipeline, uint32 amount, MiscData data) override;
		void RenderImGui() override;


		void CopyToSwapchain(Shared<Image> image) override;

	private:
		RendererConfig m_Config;

		Shared<VulkanGraphicsContext> m_GraphicsContext;
		Shared<VulkanDevice> m_Device;
		Shared<VulkanSwapchain> m_Swapchain;

		std::vector<Shared<VulkanDeviceCmdBuffer>> m_CmdBuffers;
		Shared<VulkanDeviceCmdBuffer> m_CurrentCmdBuffer;

		std::shared_mutex m_Mutex;

		static VkDescriptorPool s_DescriptorPool;
	};

}
