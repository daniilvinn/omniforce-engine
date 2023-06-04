#include "../VulkanSwapchain.h"
#include "../VulkanGraphicsContext.h"
#include "../VulkanDevice.h"
#include "../VulkanImage.h"

#include <GLFW/glfw3.h>

namespace Omni {

	VulkanSwapchain::VulkanSwapchain(const SwapchainSpecification& spec)
		: m_Surface(VK_NULL_HANDLE), m_Swapchain(VK_NULL_HANDLE), m_Specification(spec)
	{

	}

	VulkanSwapchain::~VulkanSwapchain()
	{

	}

	void VulkanSwapchain::CreateSurface(const SwapchainSpecification& spec)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VulkanGraphicsContext* ctx = VulkanGraphicsContext::Get();
		VK_CHECK_RESULT(
			glfwCreateWindowSurface(
				ctx->GetVulkanInstance(),
				(GLFWwindow*)spec.main_window->Raw(),
				nullptr,
				&m_Surface
			)
		);
		OMNIFORCE_CORE_TRACE("Created window surface");

		// Looking for available presentation modes.
		// FIFO (aka v-synced) is guaranteed to be available, so only looking for Mailbox (non v-synced) mode.
		uint32 present_mode_count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device->GetPhysicalDevice()->Raw(), m_Surface, &present_mode_count, nullptr);

		std::vector<VkPresentModeKHR> present_modes(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device->GetPhysicalDevice()->Raw(), m_Surface, &present_mode_count, present_modes.data());

		m_SupportsMailboxPresentation = false;
		for (auto& mode : present_modes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR) m_SupportsMailboxPresentation = true;
		}

		// Looking for suitable surface color space
		uint32 surface_format_count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device->GetPhysicalDevice()->Raw(), m_Surface, &surface_format_count, nullptr);

		std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device->GetPhysicalDevice()->Raw(), m_Surface, &surface_format_count, surface_formats.data());

		// To begin, set surface format as first available, if no srgb 32-bit format is supported.
		m_SurfaceFormat = surface_formats[0];

		for (auto& format : surface_formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				m_SurfaceFormat = format;
		}
	}

	void VulkanSwapchain::CreateSwapchain(const SwapchainSpecification& spec)
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		m_Specification = spec;
		m_Images.reserve(m_Specification.frames_in_flight);
		m_Semaphores.reserve(3);

		if (m_Swapchain) [[likely]]
		{
			vkDestroySwapchainKHR(device->Raw(), m_Swapchain, nullptr);
		}

		uvec2 extent = (uvec2)spec.extent;
		if (extent.x + extent.y == 0) {
			extent = { 1, 1 };
		}

		VkSurfaceCapabilitiesKHR surface_capabilities = {};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice()->Raw(), m_Surface, &surface_capabilities);

		VkSwapchainCreateInfoKHR swapchain_create_info = {};
		swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_create_info.surface = m_Surface;
		swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
		swapchain_create_info.imageFormat = m_SurfaceFormat.format;
		swapchain_create_info.imageColorSpace = m_SurfaceFormat.colorSpace;
		swapchain_create_info.minImageCount = spec.frames_in_flight;
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_create_info.imageExtent = { extent.x, extent.y };
		swapchain_create_info.imageArrayLayers = 1;
		swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_create_info.clipped = VK_TRUE;
		swapchain_create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchain_create_info.preTransform = surface_capabilities.currentTransform;
		swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

		if (m_SupportsMailboxPresentation) {
			swapchain_create_info.presentMode = m_Specification.vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
		}

		VK_CHECK_RESULT(vkCreateSwapchainKHR(device->Raw(), &swapchain_create_info, nullptr, &m_Swapchain));

		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device->Raw(), m_Swapchain, (uint32*)&m_Specification.frames_in_flight, nullptr));

		m_Images.reserve(m_Specification.frames_in_flight);
		std::vector<VkImage> pure_swapchain_images(m_Specification.frames_in_flight);

		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device->Raw(), m_Swapchain, (uint32*)&m_Specification.frames_in_flight, pure_swapchain_images.data()));

		VkCommandBuffer image_layout_transition_command_buffer = device->AllocateTransientCmdBuffer();

		for (auto& image : m_Images) {
			vkDestroyImageView(device->Raw(), image->RawView(), nullptr);
		}

		m_Images.clear();

		for (auto& image : pure_swapchain_images) {
			VkImageView image_view = VK_NULL_HANDLE;

			VkImageViewCreateInfo image_view_create_info = {};
			image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			image_view_create_info.image = image;
			image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			image_view_create_info.format = m_SurfaceFormat.format;
			image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_view_create_info.subresourceRange.baseArrayLayer = 0;
			image_view_create_info.subresourceRange.layerCount = 1;
			image_view_create_info.subresourceRange.baseMipLevel = 0;
			image_view_create_info.subresourceRange.levelCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(device->Raw(), &image_view_create_info, nullptr, &image_view));

			ImageSpecification swapchain_image_spec = {};
			swapchain_image_spec.extent = { (uint32)m_Specification.extent.x, (uint32)m_Specification.extent.y, 1 };
			swapchain_image_spec.usage = ImageUsage::RENDER_TARGET;
			swapchain_image_spec.type = ImageType::TYPE_2D;
			swapchain_image_spec.format = convert(m_SurfaceFormat.format);

			m_Images.push_back(std::make_shared<VulkanImage>(swapchain_image_spec, image, image_view));

			VkImageMemoryBarrier image_memory_barrier = {};
			image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			image_memory_barrier.image = image;
			image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_memory_barrier.subresourceRange.baseArrayLayer = 0;
			image_memory_barrier.subresourceRange.layerCount = 1;
			image_memory_barrier.subresourceRange.baseMipLevel = 0;
			image_memory_barrier.subresourceRange.levelCount = 1;

			vkCmdPipelineBarrier(
				image_layout_transition_command_buffer,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&image_memory_barrier
			);
		}

		device->ExecuteTransientCmdBuffer(image_layout_transition_command_buffer);

		for (auto& image : m_Images) {
			image->SetCurrentLayout(ImageLayout::PRESENT_SRC);
		}

		m_CurrentFrameIndex = 0;

		/*  ===================
		*	Create sync objects
		*/

		VkSemaphoreCreateInfo semaphore_create_info = {};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (int32 i = 0; i < m_Specification.frames_in_flight; i++) {
			VkSemaphore render_semaphore;
			VkSemaphore present_semaphore;

			VK_CHECK_RESULT(vkCreateSemaphore(device->Raw(), &semaphore_create_info, nullptr, &render_semaphore));
			VK_CHECK_RESULT(vkCreateSemaphore(device->Raw(), &semaphore_create_info, nullptr, &present_semaphore));

			m_Semaphores.push_back({ render_semaphore, present_semaphore });
		}

		m_Fences.reserve(spec.frames_in_flight);

		m_Semaphores.shrink_to_fit();
		m_Fences.shrink_to_fit();

		VkFenceCreateInfo fence_create_info = {};
		fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int32 i = 0; i < spec.frames_in_flight; i++) {
			VkFence fence;
			VK_CHECK_RESULT(vkCreateFence(device->Raw(), &fence_create_info, nullptr, &fence));
			m_Fences.push_back(fence);
		}
		
		OMNIFORCE_CORE_TRACE(
			"Created renderer swapchain. Spec - extent: {0}x{1}, VSync: {2}, frames in flight: {3}",
			spec.extent.x,
			spec.extent.y,
			spec.vsync ? "on" : "off",
			spec.frames_in_flight
		);
	}

	void VulkanSwapchain::CreateSwapchain()
	{

	}

	void VulkanSwapchain::DestroySurface()
	{
		VulkanGraphicsContext* ctx = VulkanGraphicsContext::Get();

		OMNIFORCE_ASSERT_TAGGED(!m_Swapchain, "Attempted to destroy window surface, but associated swapchain was not destroyed yet");
		vkDestroySurfaceKHR(ctx->GetVulkanInstance(), m_Surface, nullptr);
	}

	void VulkanSwapchain::DestroySwapchain()
	{
		Shared<VulkanDevice> device = VulkanGraphicsContext::Get()->GetDevice();

		vkDeviceWaitIdle(device->Raw());
		for (auto& image : m_Images) {
			vkDestroyImageView(device->Raw(), image->RawView(), nullptr);
		}

		vkDestroySwapchainKHR(device->Raw(), m_Swapchain, nullptr);

		for (auto& semaphores : m_Semaphores) {
			vkDestroySemaphore(device->Raw(), semaphores.render_complete, nullptr);
			vkDestroySemaphore(device->Raw(), semaphores.present_complete, nullptr);
		}

		for (auto& fence : m_Fences) {
			vkDestroyFence(device->Raw(), fence, nullptr);
		}

		m_Swapchain = VK_NULL_HANDLE;
	}

	void VulkanSwapchain::SetVSync(bool vsync)
	{
		if (m_Specification.vsync == vsync) return;

		m_Specification.vsync = vsync;

		SwapchainSpecification current_spec = GetSpecification();
		SwapchainSpecification new_spec = current_spec;
		new_spec.vsync = vsync;

		CreateSwapchain(new_spec);
	}

	void VulkanSwapchain::BeginFrame()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		vkWaitForFences(device->Raw(), 1, &m_Fences[m_CurrentFrameIndex], VK_TRUE, UINT64_MAX);
		vkResetFences(device->Raw(), 1, &m_Fences[m_CurrentFrameIndex]);

		VkResult acquisition_result = vkAcquireNextImageKHR(
			device->Raw(),
			m_Swapchain,
			UINT64_MAX,
			m_Semaphores[m_CurrentFrameIndex].present_complete,
			VK_NULL_HANDLE,
			&m_CurrentImageIndex
		);

		if (acquisition_result == VK_ERROR_OUT_OF_DATE_KHR || acquisition_result == VK_SUBOPTIMAL_KHR)
		{
			VkSurfaceCapabilitiesKHR surface_capabilities = {};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice()->Raw(), m_Surface, &surface_capabilities);

			SwapchainSpecification new_spec = GetSpecification();
			new_spec.extent = { (int32)surface_capabilities.currentExtent.width, (int32)surface_capabilities.currentExtent.height };

			vkQueueWaitIdle(device->GetAsyncComputeQueue());

			CreateSwapchain(new_spec);
		}

	}

	void VulkanSwapchain::EndFrame()
	{
		auto device = VulkanGraphicsContext::Get()->GetDevice();

		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pImageIndices = &m_CurrentImageIndex;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &m_Swapchain;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &m_Semaphores[m_CurrentFrameIndex].render_complete;
		present_info.pResults = nullptr;

		VkResult present_result = vkQueuePresentKHR(device->GetGeneralQueue(), &present_info);

		if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
		{
			VkSurfaceCapabilitiesKHR surface_capabilities = {};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice()->Raw(), m_Surface, &surface_capabilities);

			SwapchainSpecification new_spec = GetSpecification();
			new_spec.extent = { (int32)surface_capabilities.currentExtent.width, (int32)surface_capabilities.currentExtent.height };

			vkQueueWaitIdle(device->GetGeneralQueue());

			CreateSwapchain(new_spec);
		}
		
		
		m_CurrentFrameIndex = (m_CurrentImageIndex + 1) % m_Specification.frames_in_flight;
	}

}