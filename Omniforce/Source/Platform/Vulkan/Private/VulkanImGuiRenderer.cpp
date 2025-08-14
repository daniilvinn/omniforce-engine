#include <Foundation/Common.h>
#include <Platform/Vulkan/UI/VulkanImGuiRenderer.h>

#include <Platform/Vulkan/VulkanDeviceCmdBuffer.h>
#include <Platform/Vulkan/VulkanGraphicsContext.h>
#include <Platform/Vulkan/VulkanImage.h>

#include <GLFW/glfw3.h>
#include <robin_hood.h>
#include <ImGuizmo.h>
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"

namespace Omni {

	static VkDescriptorPool pool = VK_NULL_HANDLE;

	static robin_hood::unordered_map<UUID, VkDescriptorSet> imgui_image_descriptor_sets;

	VulkanImGuiRenderer::VulkanImGuiRenderer()
	{

	}

	VulkanImGuiRenderer::~VulkanImGuiRenderer()
	{

	}

	void VulkanImGuiRenderer::Launch(void* window_handle)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();
		auto context = VulkanGraphicsContext::Get();
		auto glfw_window = (GLFWwindow*)window_handle;

		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VK_CHECK_RESULT(vkCreateDescriptorPool(device->Raw(), &pool_info, nullptr, &pool));

		OMNIFORCE_CORE_TRACE("Created ImGui renderer descriptor pool");

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		// UE5-like dark theme and spacing
		ImGuiStyle& style = ImGui::GetStyle();
		style.Alpha = 1.0f;
		style.WindowPadding = ImVec2(8.0f, 6.0f);
		style.FramePadding = ImVec2(10.0f, 6.0f);
		style.ItemSpacing = ImVec2(8.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
		style.IndentSpacing = 18.0f;
		style.ScrollbarSize = 14.0f;
		style.GrabMinSize = 12.0f;
		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.TabBorderSize = 0.0f;
		style.WindowRounding = 4.0f;
		style.FrameRounding = 2.0f;
		style.GrabRounding = 2.0f;
		style.TabRounding = 3.0f;
		style.ScrollbarRounding = 2.0f;
		style.WindowMenuButtonPosition = ImGuiDir_None;

		// For multi-viewport, keep platform windows perfectly flat
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Core palette
		const ImVec4 accent = ImVec4(0.06f, 0.49f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.97f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.98f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.13f, 0.14f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.06f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.06f, 0.60f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.04f, 0.05f, 0.54f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.33f, 0.33f, 0.36f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = accent;
		style.Colors[ImGuiCol_SliderGrab] = accent;
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.12f, 0.56f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = accent;
		style.Colors[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.30f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.80f);
		style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(accent.x, accent.y, accent.z, 0.78f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(accent.x, accent.y, accent.z, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_Tab] = ImVec4(0.09f, 0.09f, 0.10f, 0.95f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(accent.x, accent.y, accent.z, 0.80f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
		style.Colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
		style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
		style.Colors[ImGuiCol_DragDropTarget] = accent;
		style.Colors[ImGuiCol_NavHighlight] = accent;
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

		VkInstance inst = context->GetVulkanInstance();

		ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vk_instance) {
			return vkGetInstanceProcAddr(volkGetLoadedInstance(), function_name); },
			&inst
		);

		ImGui_ImplGlfw_InitForVulkan(glfw_window, true);

		VkFormat format[1] = { VK_FORMAT_B8G8R8A8_UNORM };

		VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {};
		pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		pipeline_rendering_create_info.colorAttachmentCount = 1;
		pipeline_rendering_create_info.pColorAttachmentFormats = format;

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = context->GetVulkanInstance();
		init_info.PhysicalDevice = device->GetPhysicalDevice()->Raw();
		init_info.Device = device->Raw();
		init_info.QueueFamily = device->GetPhysicalDevice()->GetQueueFamilyIndices().graphics;
		init_info.Queue = device->GetGeneralQueue();
		init_info.DescriptorPool = pool;
		init_info.MinImageCount = Renderer::GetConfig().frames_in_flight;
		init_info.ImageCount = Renderer::GetConfig().frames_in_flight;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.UseDynamicRendering = true;
		init_info.PipelineRenderingCreateInfo = pipeline_rendering_create_info;

		ImGui_ImplVulkan_Init(&init_info);

		ImFont* m_MainFont = io.Fonts->AddFontFromFileTTF("Resources/Fonts/roboto.ttf", 16);
		ImGui_ImplVulkan_CreateFontsTexture();

		if (m_MainFont == nullptr)
			OMNIFORCE_CORE_CRITICAL("Failed to load font for ImGui renderer");

		OMNIFORCE_CORE_INFO("Initialized ImGui renderer");
	}

	void VulkanImGuiRenderer::Destroy()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		vkDeviceWaitIdle(device->Raw());

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		vkDestroyDescriptorPool(device->Raw(), pool, nullptr);
	}

	void VulkanImGuiRenderer::BeginFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void VulkanImGuiRenderer::EndFrame()
	{
		OnRender();
		Renderer::Submit([]() {
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		});

	}


	void VulkanImGuiRenderer::OnRender()
	{
		Renderer::RenderImGui();
	}

	namespace UI {
		void UnregisterImage(Ref<Image> image) {
			imgui_image_descriptor_sets.erase(image->Handle);
		}

		void RenderImage(Ref<Image> image, Ref<ImageSampler> sampler, ImVec2 size, uint32 image_layer, bool flip) {
			Ref<VulkanImage> vk_image = image;
			Ref<VulkanImageSampler> vk_sampler = sampler;

			if (imgui_image_descriptor_sets.find(image->Handle) == imgui_image_descriptor_sets.end()) {

				VkDescriptorSet imgui_image_id = ImGui_ImplVulkan_AddTexture(
					vk_sampler->Raw(),
					vk_image->RawView(),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);
				imgui_image_descriptor_sets.emplace(image->Handle, imgui_image_id);
			}
			ImGui::Image(imgui_image_descriptor_sets[image->Handle], size, { 0, (float32)!flip }, { 1, (float32)flip });
		};

		bool RenderImageButton(Ref<Image> image, Ref<ImageSampler> sampler, ImVec2 size, uint32 image_layer, bool flip) {
			Ref<VulkanImage> vk_image = image;
			Ref<VulkanImageSampler> vk_sampler = sampler;

			if (imgui_image_descriptor_sets.find(image->Handle) == imgui_image_descriptor_sets.end()) {

				VkDescriptorSet imgui_image_id = ImGui_ImplVulkan_AddTexture(
					vk_sampler->Raw(),
					vk_image->RawView(),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);
				imgui_image_descriptor_sets.emplace(image->Handle, imgui_image_id);
			}
			return ImGui::ImageButton(imgui_image_descriptor_sets[image->Handle], size, { 0, (float32)!flip }, { 1, (float32)flip });
		}

	}

}	