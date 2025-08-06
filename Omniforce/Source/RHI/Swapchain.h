#pragma once

#include <Foundation/Common.h>
#include <Core/Windowing/AppWindow.h>

#include <RHI/Image.h>

namespace Omni {

	struct SwapchainSpecification {
		AppWindow* main_window;
		ivec2 extent;
		int32 frames_in_flight;
		bool vsync;
		bool use_depth;
	};

	class OMNIFORCE_API Swapchain {
	public:
		virtual ~Swapchain() {};

		virtual void CreateSurface(const SwapchainSpecification& spec) = 0;
		virtual void CreateSwapchain(const SwapchainSpecification& spec) = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual bool IsVSync() const = 0;
		virtual void SetVSync(bool vsync) = 0;

		virtual Ref<Image> GetCurrentImage() = 0;

		virtual SwapchainSpecification GetSpecification() = 0;

	private:

	};

}