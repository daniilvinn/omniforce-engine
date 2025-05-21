#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <Platform/Vulkan/VulkanDeviceCmdBuffer.h>
#include <Renderer/RendererAPI.h>
#include <Renderer/Swapchain.h>
#include <Platform/Vulkan/VulkanSwapchain.h>

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
		uint32 GetDeviceOptimalTaskWorkGroupSize() const override;
		uint32 GetDeviceOptimalMeshWorkGroupSize() const override;
		uint32 GetDeviceOptimalComputeWorkGroupSize() const override;
		RendererCapabilities GetCapabilities() const override;
		Ref<DeviceCmdBuffer> GetCmdBuffer() override { return m_CurrentCmdBuffer; };
		Ref<Swapchain> GetSwapchain() override { return m_Swapchain; };

		void BeginFrame() override;
		void EndFrame() override;
		void BeginRender(const std::vector<Ref<Image>> attachments, uvec3 render_area, ivec2 render_offset, fvec4 clear_color, bool clear_depth) override;
		void EndRender(Ref<Image> target) override;
		void WaitDevice() override;
		void BindSet(Ref<DescriptorSet> set, Ref<Pipeline> pipeline, uint8 index) override;
		void BindSet(Ref<DescriptorSet> set, Ref<RTPipeline> pipeline, uint8 index) override;
		void CopyToSwapchain(Ref<Image> image) override;
		void InsertBarrier(const PipelineBarrierInfo& barrier) override;

		void ClearImage(Ref<Image> image, const fvec4& value) override;
		void RenderMeshTasks(Ref<Pipeline> pipeline, const glm::uvec3& dimensions, MiscData data) override;
		void RenderMeshTasksIndirect(Ref<Pipeline> pipeline, Ref<DeviceBuffer> params, MiscData data) override;
		void RenderQuad(Ref<Pipeline> pipeline, MiscData data) override;
		void RenderQuad(Ref<Pipeline> pipeline, uint32 amount, MiscData data) override;
		void DispatchCompute(Ref<Pipeline> pipeline, const glm::uvec3& dimensions, MiscData data) override;
		void RenderUnindexed(Ref<Pipeline> pipeline, Ref<DeviceBuffer> vertex_buffer, MiscData data) override;
		void DispatchRayTracing(Ref<RTPipeline> pipeline, const glm::uvec3& grid, MiscData data) override;

		void RenderImGui() override;

		void BeginCommandRecord() override;
		void EndCommandRecord() override;
		void ExecuteCurrentCommands() override;
		static std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout layout, uint32 count);
		static void FreeDescriptorSets(std::vector<VkDescriptorSet> sets);

		static VkDescriptorPool GetGlobalDescriptorPool() {
			return s_DescriptorPool;
		}

		static std::shared_mutex& GetMutex() {
			return m_Mutex;
		}

	private:
		RendererConfig m_Config;
		RendererCapabilities m_Caps;

		Ref<VulkanGraphicsContext> m_GraphicsContext;
		Ref<VulkanDevice> m_Device;
		Ref<VulkanSwapchain> m_Swapchain;

		std::vector<Ref<VulkanDeviceCmdBuffer>> m_CmdBuffers;
		Ref<VulkanDeviceCmdBuffer> m_CurrentCmdBuffer;

		inline static VkDescriptorPool s_DescriptorPool = VK_NULL_HANDLE;

		inline static std::shared_mutex m_Mutex;
	};

}
