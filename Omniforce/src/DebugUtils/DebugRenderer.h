#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Renderer/DeviceBuffer.h>
#include <Renderer/Image.h>
#include <Renderer/Pipeline.h>
#include <Scene/SceneRendererPrimitives.h>

#include <glm/glm.hpp>

namespace Omni {

	class OMNIFORCE_API DebugRenderer {
	public:
		static void Init();
		static void Shutdown();

		static void SetCameraBuffer(Shared<DeviceBuffer> buffer);

		static void RenderWireframeSphere(const glm::vec3& position, float radius, const glm::vec3& color);
		static void RenderWireframeBox(const glm::vec3& translation, const glm::quat rotation, const glm::vec3 scale, const glm::vec3& color);

		static void Render(Shared<Image> target);

	private:
		DebugRenderer();
		~DebugRenderer();

	private:
		struct PushConstants {
			uint64 camera_data_bda;
			TRS trs;
			glm::u8vec3 lines_color;
		};

		inline static DebugRenderer* renderer;

		Shared<Pipeline> m_WireframePipeline;
		Shared<DeviceBuffer> m_CameraBuffer;
		Shared<DeviceBuffer> m_IcosphereMesh;
		Shared<DeviceBuffer> m_CubeMesh;

		std::vector<std::function<void()>> m_DebugRequests;

	};

}