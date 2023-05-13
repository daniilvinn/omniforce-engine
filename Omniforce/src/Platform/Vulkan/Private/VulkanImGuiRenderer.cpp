#include <Platform/Vulkan/UI/VulkanImGuiRenderer.h>

#include "../VulkanGraphicsContext.h"
#include "../VulkanImage.h"

#include <GLFW/glfw3.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <robin_hood.h>

namespace Omni {

	static VkDescriptorPool pool = VK_NULL_HANDLE;

	static robin_hood::unordered_map<uint32, VkDescriptorSet> imgui_image_descriptor_sets;

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

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.FrameRounding = 5;
		style.FramePadding = { 5, 5 };
		style.WindowPadding = { 5, 5 };
		style.Colors[ImGuiCol_Button] = ImVec4(0.2, 0.2, 0.2, 1);

		ImGui_ImplGlfw_InitForVulkan(glfw_window, true);
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
		ImGui_ImplVulkan_Init(&init_info, VK_FORMAT_B8G8R8A8_UNORM);
;
		ImFont* m_MainFont = io.Fonts->AddFontFromFileTTF("assets/fonts/roboto.ttf", 16);

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		auto cmd_buffer = device->AllocateTransientCmdBuffer();
		ImGui_ImplVulkan_CreateFontsTexture(cmd_buffer);
		device->ExecuteTransientCmdBuffer(cmd_buffer);

		ImGui_ImplVulkan_DestroyFontUploadObjects();

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
		void RenderImage(Shared<Image> image, Shared<ImageSampler> sampler, ImVec2 size, uint32 image_layer) {
			Shared<VulkanImage> vk_image = ShareAs<VulkanImage>(image);
			Shared<VulkanImageSampler> vk_sampler = ShareAs<VulkanImageSampler>(sampler);

			if (imgui_image_descriptor_sets.find(image->GetId()) == imgui_image_descriptor_sets.end()) {
				VkDescriptorSet imgui_image_id = ImGui_ImplVulkan_AddTexture(
					vk_sampler->Raw(), 
					vk_image->RawView(), 
					vk_image->GetCurrentLayout()
				);
				imgui_image_descriptor_sets.emplace(image->GetId(), imgui_image_id);
			}
			ImGui::Image(imgui_image_descriptor_sets[image->GetId()], size, {0, 1}, {1, 0});
		};
	}

}	