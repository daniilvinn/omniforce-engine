#pragma once

#include <Foundation/Common.h>

#include <Renderer/DeviceBuffer.h>
#include <Renderer/Image.h>
#include <Renderer/Pipeline.h>
#include <Scene/SceneRendererPrimitives.h>
#include <Shaders/Shared/Transform.glslh>

#include <glm/glm.hpp>

namespace Omni {

	enum class DebugSceneView {
		CLUSTER,
		TRIANGLE,
		NONE
	};

	class OMNIFORCE_API DebugRenderer {
	public:
		static void Init();
		static void Shutdown();

		static void SetCameraBuffer(Ref<DeviceBuffer> buffer);

		static void RenderWireframeSphere(const glm::vec3& position, float radius, const glm::vec3& color);
		static void RenderWireframeBox(const glm::vec3& translation, const glm::quat rotation, const glm::vec3 scale, const glm::vec3& color);
		static void RenderWireframeLines(Ref<DeviceBuffer> vbo, const glm::vec3& translation, const glm::quat rotation, const glm::vec3 scale, const glm::vec3& color);

		static void Render(Ref<Image> target, Ref<Image> depth_target, fvec4 clear_value);

		static void RenderSceneDebugView(Ref<DeviceBuffer> visible_clusters, DebugSceneView mode, Ref<DescriptorSet> descriptor_set);

	private:
		DebugRenderer();
		~DebugRenderer();

	private:
		struct WireframePushConstants {
			uint64 camera_data_bda;
			GLSL::Transform trs;
			glm::u8vec3 lines_color;
		};

		inline static DebugRenderer* renderer;

		Ref<Pipeline> m_WireframePipeline;
		Ref<Pipeline> m_DebugViewPipeline;
		Ref<DeviceBuffer> m_CameraBuffer;
		Ref<DeviceBuffer> m_IcosphereMesh;
		Ref<DeviceBuffer> m_CubeMesh;

		std::vector<std::function<void()>> m_DebugRequests;

	};

}