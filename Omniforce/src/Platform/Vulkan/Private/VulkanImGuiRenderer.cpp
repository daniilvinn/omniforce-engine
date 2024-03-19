#include <Platform/Vulkan/UI/VulkanImGuiRenderer.h>

#include "../VulkanDeviceCmdBuffer.h"
#include "../VulkanGraphicsContext.h"
#include "../VulkanImage.h"

#include <GLFW/glfw3.h>
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"
#include <robin_hood.h>
#include <ImGuizmo.h>

namespace Omni {

	static VkDescriptorPool pool = VK_NULL_HANDLE;

	static robin_hood::unordered_map<UUID, VkDescriptorSet> imgui_image_descriptor_sets;

	VulkanImGuiRenderer::VulkanImGuiRenderer()
	{
		OMNIFORCE_CORE_TRACE("Create ImGui renderer");
	}

	VulkanImGuiRenderer::~VulkanImGuiRenderer()
	{

	}

	void VulkanImGuiRenderer::Launch(void* window_handle)
	{
		OMNIFORCE_CORE_TRACE("Launching ImGui renderer...");
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

		OMNIFORCE_CORE_TRACE("Created ImGui context");

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.Alpha = 1.0f;
		style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
		style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
		style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		style.GrabRounding = style.FrameRounding = 2.3f;

		OMNIFORCE_CORE_TRACE("Setted ImGui styles");

		VkInstance inst = context->GetVulkanInstance();

		ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vk_instance) {
			return vkGetInstanceProcAddr(volkGetLoadedInstance(), function_name); },
			&inst
		);

		OMNIFORCE_CORE_TRACE("Loaded vulkan functions for imgui renderer");

		ImGui_ImplGlfw_InitForVulkan(glfw_window, true);

		OMNIFORCE_CORE_TRACE("Initialized ImGui GLFW implementation for Vulkan");

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

		OMNIFORCE_CORE_TRACE("[ImGuiRenderer]: Min image count: {} | image count: {}", init_info.MinImageCount, init_info.ImageCount);

		ImFont* m_MainFont = io.Fonts->AddFontFromFileTTF("resources/fonts/roboto.ttf", 16);

		ImGui_ImplVulkan_CreateFontsTexture();

		OMNIFORCE_CORE_TRACE("Launched ImGui renderer");
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
		void UnregisterImage(Shared<Image> image) {
			imgui_image_descriptor_sets.erase(image->Handle);
		}

		void RenderImage(Shared<Image> image, Shared<ImageSampler> sampler, ImVec2 size, uint32 image_layer, bool flip) {
			Shared<VulkanImage> vk_image = ShareAs<VulkanImage>(image);
			Shared<VulkanImageSampler> vk_sampler = ShareAs<VulkanImageSampler>(sampler);
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

		bool RenderImageButton(Shared<Image> image, Shared<ImageSampler> sampler, ImVec2 size, uint32 image_layer, bool flip) {
			Shared<VulkanImage> vk_image = ShareAs<VulkanImage>(image);
			Shared<VulkanImageSampler> vk_sampler = ShareAs<VulkanImageSampler>(sampler);
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