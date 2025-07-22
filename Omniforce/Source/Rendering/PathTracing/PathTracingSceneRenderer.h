#pragma once

#include <Foundation/Common.h>
#include <Scene/SceneCommon.h>

#include <Rendering/ISceneRenderer.h>
#include <Scene/Camera.h>
#include <RHI/AccelerationStructure.h>
#include <RHI/RTPipeline.h>

#include <glm/glm.hpp>

namespace Omni {

	class PathTracingSceneRenderer : public ISceneRenderer {
	public:
		static Ref<ISceneRenderer> Create(IAllocator* allocator, SceneRendererSpecification& spec);

		PathTracingSceneRenderer(const SceneRendererSpecification& spec);
		~PathTracingSceneRenderer();

		void BeginScene(Ref<Camera> camera) override;
		void EndScene() override;

	private:
		Ref<RTPipeline> m_RTPipeline;
		Ref<Pipeline> m_ToneMappingPass;

		Ptr<RTAccelerationStructure> m_SceneTLAS;
		Ptr<RTAccelerationStructure> m_SphereBLAS;

		Ref<Image> m_VisibilityBuffer;
		Ref<Image> m_OutputImage;
		ViewData m_PreviousFrameView = {};
		uint64 m_AccumulatedFrameCount = 0;
		float32 m_Exposure = 1.0f;

	};

}