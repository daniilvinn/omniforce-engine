#pragma once

#include "VulkanCommon.h"

#include <Renderer/Swapchain.h>

namespace Cursed {

	class VulkanSwapchain : public Swapchain {
	public:
		struct SwapchainSemaphores {
			VkSemaphore render_complete;
			VkSemaphore present_complete;
		};

		VulkanSwapchain(const SwapchainSpecification& spec);
		~VulkanSwapchain();

		void CreateSurface(const SwapchainSpecification& spec) override;
		void CreateSwapchain(const SwapchainSpecification& spec) override;
		void CreateSwapchain() override;

		void DestroySurface() override;
		void DestroySwapchain() override;
		
		void BeginFrame() override;
		void EndFrame() override;

		VkSurfaceKHR RawSurface() const { return m_Surface; }
		VkSwapchainKHR RawSwapchain() const { return m_Swapchain; }

		bool IsVSync() const override { return m_Specification.vsync; };
		void SetVSync(bool vsync) override;

		SwapchainSpecification GetSpecification() override { return m_Specification; }
		uint32 GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

		SwapchainSemaphores GetSemaphores() const { return m_Semaphores[m_CurrentFrameIndex]; }

	private:
		SwapchainSpecification m_Specification;
		VkSurfaceKHR m_Surface;
		VkSwapchainKHR m_Swapchain;

		VkSurfaceFormatKHR m_SurfaceFormat;
		VkPresentModeKHR m_CurrentPresentMode;
		bool m_SupportsMailboxPresentation;

		std::vector<Shared<VulkanImage>> m_Images;

		std::vector<SwapchainSemaphores> m_Semaphores;
		std::vector<VkFence> m_Fences;

		uint32 m_CurrentFrameIndex;
		uint32 m_CurrentImageIndex;
	};

}