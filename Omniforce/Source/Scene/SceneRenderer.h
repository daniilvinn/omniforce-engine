#pragma once

#include <Foundation/Common.h>
#include <Scene/SceneCommon.h>

#include <Scene/Sprite.h>
#include <Scene/Camera.h>
#include <Scene/DeviceMaterialPool.h>
#include <Scene/SceneRendererPrimitives.h>
#include <Scene/DeviceIndexedResourceBuffer.h>
#include <Scene/Lights.h>
#include <Renderer/Renderer.h>
#include <Renderer/Image.h>
#include <Renderer/Mesh.h>
#include <Core/CallbackRHUMap.h>
#include <DebugUtils/DebugRenderer.h>
#include <Shaders/Shared/RenderObject.glslh>
#include <Shaders/Shared/MeshData.glslh>
#include <Shaders/Shared/Lights.glslh>

#include <shared_mutex>

#include <robin_hood.h>
#include <glm/glm.hpp>

namespace robin_hood {
	template<>
	struct hash<Omni::Sprite> {

	};
}

namespace Omni {

	struct OMNIFORCE_API SceneRendererSpecification {
		uint8 anisotropic_filtering = 16;
		// How much sprites will be stored in a device buffer. Actual size will be sprite_buffer_size * sizeof(Sprite) * fif
		uint32 sprite_buffer_size = 2500;
	};

	class OMNIFORCE_API SceneRenderer {
	public:
		static Ref<SceneRenderer> Create(IAllocator* allocator, SceneRendererSpecification& spec);

		SceneRenderer(const SceneRendererSpecification& spec);
		~SceneRenderer();

		void BeginScene(Ref<Camera> camera);
		void EndScene();

		void EnterDebugMode(DebugSceneView mode) { m_CurrentDebugMode = mode; };
		void ExitDebugMode() { m_CurrentDebugMode = DebugSceneView::NONE; };
		bool IsInDebugMode() const { return m_CurrentDebugMode != DebugSceneView::NONE; }
		DebugSceneView GetCurrentDebugMode() const { return m_CurrentDebugMode; }

		Ref<Image> GetFinalImage();
		Ref<ImageSampler> GetSamplerNearest() { return m_SamplerNearest; }
		Ref<ImageSampler> GetSamplerLinear() { return m_SamplerLinear; }
		AssetHandle GetDummyWhiteTexture() const { return m_DummyWhiteTexture->Handle; }
		/*
		* @brief Adds resource to a global renderer data
		* @return returns an index the resource can be accessed with
		*/
		uint32 AcquireResourceIndex(Ref<Image> image, SamplerFilteringMode filtering_mode);
		uint32 AcquireResourceIndex(Ref<Image> image);
		uint32 AcquireResourceIndex(Ref<Mesh> mesh);
		uint32 AcquireResourceIndex(Ref<Material> material); // WARNING: this returns offset in material pool, not index

		/*
		* @brief Removes resource to a global renderer data
		* @return true if successful, false if no resource found
		*/
		bool ReleaseResourceIndex(Ref<Image> image);
		uint32 GetTextureIndex(const AssetHandle& uuid) const { return m_TextureIndices.at(uuid); }
		uint64 GetMaterialBDA(const AssetHandle& id) const { return m_MaterialDataPool.GetStorageBufferAddress() + m_MaterialDataPool.GetOffset(id); }
		uint32 GetMeshIndex(const AssetHandle& uuid) const { return m_MeshResourcesBuffer.GetIndex(uuid); }

		void RenderSprite(const Sprite& sprite);
		void RenderObject(Ref<Pipeline> pipeline, const GLSL::RenderObjectData& render_data);

		// Lighting
		void AddPointLight(const GLSL::PointLight& point_light) { m_HostPointLights.push_back(point_light); }

	private:
		Ref<Camera> m_Camera;
		SceneRendererSpecification m_Specification;

		std::vector<Ref<Image>> m_RendererOutputs;
		std::vector<Ref<Image>> m_DepthAttachments;
		Ref<Image> m_CurrectMainRenderTarget;
		Ref<Image> m_CurrentDepthAttachment;

		std::vector<Ref<DescriptorSet>> m_SceneDescriptorSet;
		Ref<ImageSampler> m_SamplerNearest;
		Ref<ImageSampler> m_SamplerLinear;
		Ref<Image> m_DummyWhiteTexture;
		uint32 m_SpriteBufferSize; // size in bytes per frame in flight, not overall size
		std::vector<Sprite> m_SpriteQueue;

		Ref<Pipeline> m_SpritePass;
		Ref<Pipeline> m_LinePass;
		Ref<Pipeline> m_ClearPass;

		// ~ Omni 2024 ~
		Ref<DeviceBuffer> m_CameraDataBuffer;

		Ptr<VirtualMemoryBlock> m_TextureIndexAllocator;
		Ptr<VirtualMemoryBlock> m_StorageImageIndexAllocator;
		rhumap<UUID, uint32> m_TextureIndices;
		rhumap<UUID, uint32> m_StorageImageIndices;
		Ref<DeviceBuffer> m_SpriteDataBuffer;

		DeviceIndexedResourceBuffer<GLSL::MeshData> m_MeshResourcesBuffer;
		DeviceMaterialPool m_MaterialDataPool;

		rh::unordered_flat_set<Ref<Pipeline>> m_ActiveMaterialPipelines;
		std::vector<GLSL::RenderObjectData> m_HostRenderQueue;
		Ref<DeviceBuffer> m_DeviceRenderQueue;
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

		std::vector<GLSL::PointLight> m_HostPointLights;
		Ref<DeviceBuffer> m_DevicePointLights;
		std::shared_mutex m_Mutex;

		DebugSceneView m_CurrentDebugMode = DebugSceneView::NONE;

	};

}