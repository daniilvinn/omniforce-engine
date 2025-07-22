#pragma once

#include <Foundation/Common.h>
#include <Platform/Vulkan/VulkanCommon.h>

#include <RHI/Swapchain.h>
#include <Platform/Vulkan/VulkanDevice.h>
#include <Platform/Vulkan/VulkanImage.h>

namespace Omni {

	class VulkanSwapchain : public Swapchain {
	public:
		VulkanSwapchain(const SwapchainSpecification& spec);
		~VulkanSwapchain();

		void CreateSurface(const SwapchainSpecification& spec) override;
		void CreateSwapchain(const SwapchainSpecification& spec) override;
		
		void BeginFrame() override;
		void EndFrame() override;

		VkSurfaceKHR RawSurface() const { return m_Surface; }
		VkSwapchainKHR RawSwapchain() const { return *m_Swapchain; }

		bool IsVSync() const override { return m_Specification.vsync; };
		void SetVSync(bool vsync) override;

		SwapchainSpecification GetSpecification() override { return m_Specification; }

		// New accessors
		VkSemaphore GetAcquireSemaphore() const { return m_AcquireSemaphores[m_CurrentFrame]; }
		VkSemaphore GetRenderSemaphore() const { return m_RenderSemaphores[m_CurrentImage]; }
		VkFence GetFence() const { return m_Fences[m_CurrentFrame]; }
		uint32_t GetCurrentImageIndex() const { return m_CurrentImage; }
		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
		Ref<Image> GetCurrentImage() override { return m_Images[m_CurrentImage]; };

	private:
		SwapchainSpecification m_Specification;
		VkSurfaceKHR m_Surface;
		VkSwapchainKHR* m_Swapchain; // pointer because we need to somehow access it on engine shutdown

		VkSurfaceFormatKHR m_SurfaceFormat;
		VkPresentModeKHR m_CurrentPresentMode;
		bool m_SupportsMailboxPresentation;

		std::vector<Ref<VulkanImage>> m_Images;

		// Per-image semaphores (as suggested by validation layer)
		std::vector<VkSemaphore> m_AcquireSemaphores;
		std::vector<VkSemaphore> m_RenderSemaphores;
		// Per-frame fences
		std::vector<VkFence> m_Fences;

		uint32 m_SwachainImageCount = 3;
		uint32 m_FramesInFlight = 2;

		uint32 m_CurrentFrame = 0;
		uint32 m_CurrentImage = 0;
	};

}