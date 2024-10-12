#pragma once

#include "SceneCommon.h"
#include "Sprite.h"
#include "Camera.h"
#include "DeviceMaterialPool.h"
#include "SceneRendererPrimitives.h"
#include "DeviceIndexedResourceBuffer.h"
#include "Lights.h"
#include <Renderer/Renderer.h>
#include <Renderer/Image.h>
#include <Renderer/Mesh.h>
#include <Core/CallbackRHUMap.h>
#include <Memory/VirtualMemoryBlock.h>
#include <DebugUtils/DebugRenderer.h>

#include <vector>
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
		static Shared<SceneRenderer> Create(const SceneRendererSpecification& spec);

		SceneRenderer(const SceneRendererSpecification& spec);
		~SceneRenderer();

		void Destroy();

		void BeginScene(Shared<Camera> camera);
		void EndScene();

		void EnterDebugMode(DebugSceneView mode) { m_CurrentDebugMode = mode; };
		void ExitDebugMode() { m_CurrentDebugMode = DebugSceneView::NONE; };
		bool IsInDebugMode() const { return m_CurrentDebugMode != DebugSceneView::NONE; }
		DebugSceneView GetCurrentDebugMode() const { return m_CurrentDebugMode; }

		Shared<Image> GetFinalImage();
		static Shared<ImageSampler> GetSamplerNearest() { return s_SamplerNearest; }
		static Shared<ImageSampler> GetSamplerLinear() { return s_SamplerLinear; }
		AssetHandle GetDummyWhiteTexture() const { return m_DummyWhiteTexture->Handle; }
		/*
		* @brief Adds resource to a global renderer data
		* @return returns an index the resource can be accessed with
		*/
		uint32 AcquireResourceIndex(Shared<Image> image, SamplerFilteringMode filtering_mode);
		uint32 AcquireResourceIndex(Shared<Image> image);
		uint32 AcquireResourceIndex(Shared<Mesh> mesh);
		uint32 AcquireResourceIndex(Shared<Material> material); // WARNING: this returns offset in material pool, not index

		/*
		* @brief Removes resource to a global renderer data
		* @return true if successful, false if no resource found
		*/
		bool ReleaseResourceIndex(Shared<Image> image);
		uint32 GetTextureIndex(const AssetHandle& uuid) const { return m_TextureIndices.at(uuid); }
		uint64 GetMaterialBDA(const AssetHandle& id) const { return m_MaterialDataPool.GetStorageBufferAddress() + m_MaterialDataPool.GetOffset(id); }
		uint32 GetMeshIndex(const AssetHandle& uuid) const { return m_MeshResourcesBuffer.GetIndex(uuid); }

		void RenderSprite(const Sprite& sprite);
		void RenderObject(Shared<Pipeline> pipeline, const DeviceRenderableObject& render_data);

		// Lighting
		void AddPointLight(const PointLight& point_light) { m_HostPointLights.push_back(point_light); }

	private:
		Shared<Camera> m_Camera;
		SceneRendererSpecification m_Specification;

		std::vector<Shared<Image>> m_RendererOutputs;
		std::vector<Shared<Image>> m_DepthAttachments;
		Shared<Image> m_CurrectMainRenderTarget;
		Shared<Image> m_CurrentDepthAttachment;

		std::vector<Shared<DescriptorSet>> m_SceneDescriptorSet;
		inline static Shared<ImageSampler> s_SamplerNearest;
		inline static Shared<ImageSampler> s_SamplerLinear;
		Shared<Image> m_DummyWhiteTexture;
		uint32 m_SpriteBufferSize; // size in bytes per frame in flight, not overall size
		std::vector<Sprite> m_SpriteQueue;

		Shared<Pipeline> m_SpritePass;
		Shared<Pipeline> m_LinePass;
		Shared<Pipeline> m_ClearPass;

		// ~ Omni 2024 ~
		Shared<DeviceBuffer> m_CameraDataBuffer;

		Scope<VirtualMemoryBlock> m_TextureIndexAllocator;
		Scope<VirtualMemoryBlock> m_StorageImageIndexAllocator;
		rhumap<UUID, uint32> m_TextureIndices;
		rhumap<UUID, uint32> m_StorageImageIndices;
		Shared<DeviceBuffer> m_SpriteDataBuffer;

		DeviceIndexedResourceBuffer<DeviceMeshData> m_MeshResourcesBuffer;
		DeviceMaterialPool m_MaterialDataPool;

		rh::unordered_flat_set<Shared<Pipeline>> m_ActiveMaterialPipelines;
		std::vector<DeviceRenderableObject> m_HostRenderQueue;
		Shared<DeviceBuffer> m_DeviceRenderQueue;
		Shared<DeviceBuffer> m_CulledDeviceRenderQueue;
		Shared<DeviceBuffer> m_DeviceIndirectDrawParams;

		Shared<Pipeline> m_IndirectFrustumCullPipeline;

		struct GBuffer {
			Shared<Image> positions;
			Shared<Image> normals;
			Shared<Image> base_color;
			Shared<Image> metallic_roughness_occlusion;
		} m_GBuffer;
		Shared<Pipeline> m_PBRFullscreenPipeline;

		Shared<Image> m_VisibilityBuffer;
		Shared<Pipeline> m_VisBufferPass;
		Shared<Pipeline> m_VisMaterialResolvePass;
		Shared<DeviceBuffer> m_VisibleClusters;

		std::vector<PointLight> m_HostPointLights;
		Shared<DeviceBuffer> m_DevicePointLights;
		std::shared_mutex m_Mutex;

		DebugSceneView m_CurrentDebugMode = DebugSceneView::NONE;

	};

}