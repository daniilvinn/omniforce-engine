#pragma once

#include <Foundation/Common.h>
#include <Scene/SceneCommon.h>

#include <Scene/ISceneRenderer.h>
#include <Scene/Camera.h>
#include <Shaders/Shared/RenderObject.glslh>

#include <glm/glm.hpp>

namespace Omni {

	class PathTracingSceneRenderer : public ISceneRenderer {
	public:
		static Ref<ISceneRenderer> Create(IAllocator* allocator, SceneRendererSpecification& spec);

		PathTracingSceneRenderer(const SceneRendererSpecification& spec);
		~PathTracingSceneRenderer();

		void BeginScene(Ref<Camera> camera) override;
		void EndScene() override;

		void RenderObject(Ref<Pipeline> pipeline, const GLSL::RenderObjectData& render_data) override;

	private:

	};

}