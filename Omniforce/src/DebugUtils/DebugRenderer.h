#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <Renderer/DeviceBuffer.h>
#include <Renderer/Image.h>
#include <Renderer/Pipeline.h>
#include <Scene/SceneRendererPrimitives.h>

#include <glm/glm.hpp>

namespace Omni {

	enum class DebugSceneView {
		CLUSTER,
		TRIANGLE,
		CLUSTER_GROUP,
		NONE
	};

	class OMNIFORCE_API DebugRenderer {
	public:
		static void Init();
		static void Shutdown();

		static void SetCameraBuffer(Shared<DeviceBuffer> buffer);

		static void RenderWireframeSphere(const glm::vec3& position, float radius, const glm::vec3& color);
		static void RenderWireframeBox(const glm::vec3& translation, const glm::quat rotation, const glm::vec3 scale, const glm::vec3& color);
		static void RenderWireframeLines(Shared<DeviceBuffer> vbo, const glm::vec3& translation, const glm::quat rotation, const glm::vec3 scale, const glm::vec3& color);

		static void Render(Shared<Image> target, Shared<Image> depth_target, fvec4 clear_value);

		static void RenderSceneDebugView(Shared<DeviceBuffer> camera_data, Shared<DeviceBuffer> mesh_data, Shared<DeviceBuffer> render_queue, Shared<DeviceBuffer> indirect_params, DebugSceneView mode);

	private:
		DebugRenderer();
		~DebugRenderer();

	private:
		struct WireframePushConstants {
			uint64 camera_data_bda;
			TRS trs;
			glm::u8vec3 lines_color;
		};

		inline static DebugRenderer* renderer;

		Shared<Pipeline> m_WireframePipeline;
		Shared<Pipeline> m_DebugViewPipeline;
		Shared<DeviceBuffer> m_CameraBuffer;
		Shared<DeviceBuffer> m_IcosphereMesh;
		Shared<DeviceBuffer> m_CubeMesh;

		std::vector<std::function<void()>> m_DebugRequests;

	};

}