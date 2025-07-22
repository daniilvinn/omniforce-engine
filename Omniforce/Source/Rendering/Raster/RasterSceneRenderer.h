#pragma once

#include <Foundation/Common.h>
#include <Scene/SceneCommon.h>

#include <Rendering/ISceneRenderer.h>
#include <Rendering/SceneRendererPrimitives.h>
#include <Rendering/DeviceIndexedResourceBuffer.h>
#include <Rendering/Mesh.h>
#include <Scene/Sprite.h>
#include <Scene/Camera.h>
#include <Scene/DeviceMaterialPool.h>
#include <Scene/Lights.h>
#include <RHI/Renderer.h>
#include <RHI/Image.h>
#include <Core/CallbackRHUMap.h>
#include <DebugUtils/DebugRenderer.h>

#include <shared_mutex>

#include <robin_hood.h>
#include <glm/glm.hpp>

namespace Omni {

	class OMNIFORCE_API RasterSceneRenderer : public ISceneRenderer {
	public:
		static Ref<ISceneRenderer> Create(IAllocator* allocator, SceneRendererSpecification& spec);

		RasterSceneRenderer(const SceneRendererSpecification& spec);
		~RasterSceneRenderer();

		void BeginScene(Ref<Camera> camera) override;
		void EndScene() override;

	private:
		uint32 m_SpriteBufferSize; // size in bytes per frame in flight, not overall size

		Ref<Pipeline> m_SpritePass;
		Ref<Pipeline> m_LinePass;
		Ref<Pipeline> m_ClearPass;

		// ~ Omni 2024 ~
		Ref<DeviceBuffer> m_SpriteDataBuffer;

		rh::unordered_flat_set<Ref<Pipeline>> m_ActiveMaterialPipelines;
		Ref<DeviceBuffer> m_CulledDeviceRenderQueue;
		Ref<DeviceBuffer> m_DeviceIndirectDrawParams;

		Ref<Pipeline> m_IndirectFrustumCullPipeline;

		struct GBuffer {
			Ref<Image> positions;
			Ref<Image> normals;
			Ref<Image> base_color;
			Ref<Image> metallic_roughness_occlusion;
		} m_GBuffer;
		Ref<Pipeline> m_PBRFullscreenPipeline;

		Ref<Image> m_VisibilityBuffer;
		Ref<Pipeline> m_VisBufferPass;
		Ref<Pipeline> m_VisMaterialResolvePass;
		Ref<DeviceBuffer> m_VisibleClusters;
		Ref<DeviceBuffer> m_SWRasterQueue;

	};

}