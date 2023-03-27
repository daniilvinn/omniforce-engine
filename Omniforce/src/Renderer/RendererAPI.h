#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Memory/Pointers.hpp>

#include "ForwardDecl.h"

namespace Omni {
	struct RendererConfig;

	class OMNIFORCE_API RendererAPI {
	public:
		RendererAPI() {};
		virtual ~RendererAPI() {};

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void BeginRender(Shared<Image> target, uvec2 render_area, ivec2 render_offset, fvec4 clear_color) = 0;
		virtual void EndRender(Shared<Image> target) = 0;
		virtual void WaitDevice() = 0;

		virtual void BeginCommandRecord() = 0;
		virtual void EndCommandRecord() = 0;
		virtual void ExecuteCurrentCommands() = 0;

		virtual void ClearImage(Shared<Image> image, const fvec4& value) = 0;
		virtual void RenderMesh(Shared<Pipeline> pipeline, Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo) = 0;

		virtual Shared<Swapchain> GetSwapchain() = 0;

	private:
		

	};

};