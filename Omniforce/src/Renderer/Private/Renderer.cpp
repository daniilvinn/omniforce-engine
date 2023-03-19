#include "../Renderer.h"

#include <Threading/JobSystem.h>
#include <Platform/Vulkan/VulkanRenderer.h>

namespace Omni {

	RendererAPI* Renderer::s_RendererAPI;

	struct RendererInternalData {
		std::list<Renderer::RenderFunction> cmd_generation_function_list;
	} s_InternalData;

	void Renderer::Init(const RendererConfig& config)
	{
		s_RendererAPI = new VulkanRenderer(config);
	}

	void Renderer::Shutdown()
	{
		delete s_RendererAPI;
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

	Shared<Image> Renderer::GetSwapchainImage()
	{
		return s_RendererAPI->GetSwapchain()->GetCurrentImage();
	}

	void Renderer::ClearImage(Shared<Image> image, const fvec4& value)
	{
		s_RendererAPI->ClearImage(image, value);
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