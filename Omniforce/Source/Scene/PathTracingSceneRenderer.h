#pragma once

#include <Foundation/Common.h>
#include <Scene/SceneCommon.h>

#include <Scene/ISceneRenderer.h>
#include <Scene/Camera.h>
#include <Renderer/AccelerationStructure.h>
#include <Renderer/RTPipeline.h>

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

		Ptr<RTAccelerationStructure> m_SceneTLAS;
		Ptr<RTAccelerationStructure> m_SphereBLAS;

		Ref<Image> m_VisibilityBuffer;
		ViewData m_PreviousFrameView = {};
		uint64 m_AccumulatedFrameCount;

	};

}