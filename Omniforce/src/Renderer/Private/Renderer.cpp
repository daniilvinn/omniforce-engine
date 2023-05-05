#include "../Renderer.h"

#include <Threading/JobSystem.h>
#include <Platform/Vulkan/VulkanRenderer.h>
#include "../ShaderLibrary.h"

#include <Core/Windowing/WindowSystem.h>

namespace Omni {

	RendererAPI* Renderer::s_RendererAPI;

	struct RendererInternalData {
		std::list<Renderer::RenderFunction> cmd_generation_function_list;
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

	void Renderer::BeginRender(Shared<Image> target, uvec3 render_area, ivec2 offset, fvec4 clear_value)
	{
		s_RendererAPI->BeginRender(target, render_area, offset, clear_value);
	}

	void Renderer::EndRender(Shared<Image> target) 
	{
		s_RendererAPI->EndRender(target);
	}

	void Renderer::WaitDevice()
	{
		s_RendererAPI->WaitDevice();
	}

	void Renderer::BindSet(Shared<DescriptorSet> set, Shared<Pipeline> pipeline, uint8 index)
	{
		s_RendererAPI->BindSet(set, pipeline, index);
	}

	Shared<Image> Renderer::GetSwapchainImage()
	{
		return s_RendererAPI->GetSwapchain()->GetCurrentImage();
	}

	void Renderer::ClearImage(Shared<Image> image, const fvec4& value)
	{
		s_RendererAPI->ClearImage(image, value);
	}

	void Renderer::RenderMesh(Shared<Pipeline> pipeline, Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo, MiscData data)
	{
		s_RendererAPI->RenderMesh(pipeline, vbo, ibo, data);
	}

	void Renderer::RenderQuad(Shared<Pipeline> pipeline, MiscData data)
	{
		s_RendererAPI->RenderQuad(pipeline, data);
	}

	uint32 Renderer::GetCurrentFrameIndex()
	{
		return s_RendererAPI->GetCurrentFrameIndex();
	}

	uint32 Renderer::GetDeviceMinimalAlignment()
	{
		return s_RendererAPI->GetDeviceMinimalAlignment();
	}

	void Renderer::LoadShaderPack()
	{
		ShaderLibrary::Get()->Load("assets/shaders/texture_pass.ofs");
		ShaderLibrary::Get()->Load("assets/shaders/sprite_opaque_color.ofs");
		ShaderLibrary::Get()->Load("assets/shaders/sprite_textured.ofs");
	}

	void Renderer::Render()
	{
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

}