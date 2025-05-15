#include <Scene/PathTracingSceneRenderer.h>

namespace Omni {

	Ref<ISceneRenderer> PathTracingSceneRenderer::Create(IAllocator* allocator, SceneRendererSpecification& spec)
	{

	}

	PathTracingSceneRenderer::PathTracingSceneRenderer(const SceneRendererSpecification& spec)
		: ISceneRenderer(spec)
	{

	}

	PathTracingSceneRenderer::~PathTracingSceneRenderer()
	{

	}

	void PathTracingSceneRenderer::BeginScene(Ref<Camera> camera)
	{

	}

	void PathTracingSceneRenderer::EndScene()
	{

	}

	void PathTracingSceneRenderer::RenderObject(Ref<Pipeline> pipeline, const GLSL::RenderObjectData& render_data)
	{

	}

}