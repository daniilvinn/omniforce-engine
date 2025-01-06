#include <Foundation/Common.h>
#include <Renderer/Renderer.h>

#include <Threading/JobSystem.h>
#include <Platform/Vulkan/VulkanRenderer.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/Pipeline.h>
#include <Renderer/DescriptorSet.h>
#include <Core/Windowing/WindowSystem.h>

namespace Omni {

	RendererAPI* Renderer::s_RendererAPI;

	struct RendererInternalData {
		std::vector<Renderer::RenderFunction> cmd_generation_function_list;
		Ref<ImageSampler> m_LinearSampler;
		Ref<ImageSampler> m_NearestSampler;
	} s_InternalData;

	void Renderer::Init(const RendererConfig& config)
	{
		s_RendererAPI = new VulkanRenderer(config);
		ShaderLibrary::Init();
	}

	void Renderer::Shutdown()
	{
		ShaderLibrary::Destroy();
		delete s_RendererAPI;
	}

	RendererConfig Renderer::GetConfig()
	{
		return s_RendererAPI->GetConfig();
	}

	void Renderer::Submit(RenderFunction func)
	{
		s_InternalData.cmd_generation_function_list.push_back(func);
	}

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}
	
	void Renderer::EndFrame() 
	{
		s_RendererAPI->EndFrame();
	}

	void Renderer::BeginRender(const std::vector<Ref<Image>> attachments, uvec3 render_area, ivec2 offset, fvec4 clear_value, bool clear_depth /*= true*/)
	{
		s_RendererAPI->BeginRender(attachments, render_area, offset, clear_value, clear_depth);
	}

	void Renderer::EndRender(Ref<Image> target) 
	{
		s_RendererAPI->EndRender(target);
	}

	void Renderer::WaitDevice()
	{
		s_RendererAPI->WaitDevice();
	}

	void Renderer::BindSet(Ref<DescriptorSet> set, Ref<Pipeline> pipeline, uint8 index)
	{
		s_RendererAPI->BindSet(set, pipeline, index);
	}

	Ref<Image> Renderer::GetSwapchainImage()
	{
		return s_RendererAPI->GetSwapchain()->GetCurrentImage();
	}

	Ref<DeviceCmdBuffer> Renderer::GetCmdBuffer()
	{
		return s_RendererAPI->GetCmdBuffer();
	}

	void Renderer::InsertBarrier(const PipelineBarrierInfo& barrier_info)
	{
		s_RendererAPI->InsertBarrier(barrier_info);
	}

	void Renderer::ClearImage(Ref<Image> image, const fvec4& value)
	{
		s_RendererAPI->ClearImage(image, value);
	}

	void Renderer::RenderMeshTasks(Ref<Pipeline> pipeline, const glm::uvec3 dimensions, MiscData data)
	{
		s_RendererAPI->RenderMeshTasks(pipeline, dimensions, data);
	}

	void Renderer::RenderMeshTasksIndirect(Ref<Pipeline> pipeline, Ref<DeviceBuffer> params, MiscData data)
	{
		s_RendererAPI->RenderMeshTasksIndirect(pipeline, params, data);
	}

	void Renderer::RenderQuads(Ref<Pipeline> pipeline, MiscData data)
	{
		s_RendererAPI->RenderQuad(pipeline, data);
	}

	void Renderer::RenderQuads(Ref<Pipeline> pipeline, uint32 amount, MiscData data)
	{
		s_RendererAPI->RenderQuad(pipeline, amount, data);
	}

	void Renderer::DispatchCompute(Ref<Pipeline> pipeline, const glm::uvec3& dimensions, MiscData data)
	{
		s_RendererAPI->DispatchCompute(pipeline, dimensions, data);
	}

	void Renderer::RenderUnindexed(Ref<Pipeline> pipeline, Ref<DeviceBuffer> vertex_buffer, MiscData data)
	{
		s_RendererAPI->RenderUnindexed(pipeline, vertex_buffer, data);
	}

	uint32 Renderer::GetCurrentFrameIndex()
	{
		return s_RendererAPI->GetCurrentFrameIndex();
	}

	uint32 Renderer::GetDeviceMinimalUniformBufferOffsetAlignment()
	{
		return s_RendererAPI->GetDeviceMinimalUniformBufferAlignment();
	}

	uint32 Renderer::GetDeviceMinimalStorageBufferOffsetAlignment()
	{
		return s_RendererAPI->GetDeviceMinimalStorageBufferAlignment();
	}

	uint32 Renderer::GetDeviceOptimalTaskWorkGroupSize()
	{
		return s_RendererAPI->GetDeviceOptimalTaskWorkGroupSize();
	}

	uint32 Renderer::GetDeviceOptimalMeshWorkGroupSize()
	{
		return s_RendererAPI->GetDeviceOptimalMeshWorkGroupSize();
	}

	uint32 Renderer::GetDeviceOptimalComputeWorkGroupSize()
	{
		return s_RendererAPI->GetDeviceOptimalComputeWorkGroupSize();
	}

	Ref<ImageSampler> Renderer::GetNearestSampler()
	{
		return s_InternalData.m_NearestSampler;
	}

	Ref<ImageSampler> Renderer::GetLinearSampler()
	{
		return s_InternalData.m_LinearSampler;
	}

	void Renderer::LoadShaderPack()
	{
		
	}

	void Renderer::Render()
	{
		Renderer::Submit([=]() mutable {
			auto swapchain_image = GetSwapchainImage();

			swapchain_image->SetLayout(
				GetCmdBuffer(),
				ImageLayout::PRESENT_SRC,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				PipelineStage::ALL_COMMANDS,
				(BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE,
				(BitMask)PipelineAccess::MEMORY_READ
			);
		});
		s_RendererAPI->EndCommandRecord();
		s_RendererAPI->ExecuteCurrentCommands();

		auto function_list = std::move(s_InternalData.cmd_generation_function_list);


		//JobSystem* js = JobSystem::Get();
		//js->Execute([function_list = std::move(s_InternalData.cmd_generation_function_list)]() {
			for(auto& func : function_list) {
				func();
			}
		//});
	}

	void Renderer::RenderImGui()
	{
		s_RendererAPI->RenderImGui();
	}

}