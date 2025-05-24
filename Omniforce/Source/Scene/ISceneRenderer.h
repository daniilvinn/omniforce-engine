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
#include <Asset/Model.h>
#include <CodeGeneration/Device/RenderObject.h>

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

	class OMNIFORCE_API ISceneRenderer {
	public:
		ISceneRenderer(const SceneRendererSpecification& spec);
		virtual ~ISceneRenderer();

		void EnterDebugMode(DebugSceneView mode) { m_CurrentViewMode = mode; };
		void ExitDebugMode() { m_CurrentViewMode = DebugSceneView::NONE; };
		bool IsInDebugMode() const { return m_CurrentViewMode != DebugSceneView::NONE; }
		DebugSceneView GetCurrentDebugMode() const { return m_CurrentViewMode; }

		Ref<Image> GetFinalImage() { return m_RendererOutputs[Renderer::GetCurrentFrameIndex()]; };
		Ref<ImageSampler> GetSamplerNearest() { return m_SamplerNearest; }
		Ref<ImageSampler> GetSamplerLinear() { return m_SamplerLinear; }
		AssetHandle GetDummyWhiteTexture() const { return m_DummyWhiteTexture->Handle; }

		virtual void BeginScene(Ref<Camera> camera) = 0;
		virtual void EndScene() = 0;

		uint32 AcquireResourceIndex(Ref<Image> image, SamplerFilteringMode filtering_mode);
		uint32 AcquireResourceIndex(Ref<Image> image);
		uint32 AcquireResourceIndex(Ref<Mesh> mesh);
		uint32 AcquireResourceIndex(Ref<Material> material);

		bool ReleaseResourceIndex(Ref<Image> image) {
			uint16 index = m_TextureIndices.at(image->Handle);
			m_TextureIndexAllocator->Free(sizeof(uint32) * index);
			m_TextureIndices.erase(image->Handle);

			return true;
		}

		uint32 GetTextureIndex(const AssetHandle& uuid) const { return m_TextureIndices.at(uuid); }
		uint64 GetMaterialBDA(const AssetHandle& id) const { return m_MaterialDataPool.GetStorageBufferAddress() + m_MaterialDataPool.GetOffset(id); }
		uint32 GetMeshIndex(const AssetHandle& uuid) const { return m_MeshResourcesBuffer.GetIndex(uuid); }

		virtual void RenderObject(Ref<Pipeline> pipeline, const HostInstanceRenderData& render_data) {
			InstanceRenderData device_render_data = {};
			device_render_data.transform = render_data.transform;
			device_render_data.geometry_data_id = GetMeshIndex(render_data.mesh_handle);
			device_render_data.material_address = BDA<byte>(GetMaterialBDA(render_data.material_handle));

			m_HostRenderQueue.push_back(device_render_data);
			m_HighLevelInstanceQueue.push_back(render_data);
		};
		virtual void RenderSprite(const Sprite& sprite) { m_SpriteQueue.emplace_back(sprite); }

		// Lighting
		void AddPointLight(const PointLight& point_light) { m_HostPointLights.push_back(point_light); }

	protected:
		Ref<Camera> m_Camera;
		SceneRendererSpecification m_Specification;

		std::vector<Ref<Image>> m_RendererOutputs;
		std::vector<Ref<Image>> m_DepthAttachments;
		Ref<Image> m_CurrentMainRenderTarget;
		Ref<Image> m_CurrentDepthAttachment;

		std::vector<Ref<DescriptorSet>> m_SceneDescriptorSet;
		Ref<ImageSampler> m_SamplerNearest;
		Ref<ImageSampler> m_SamplerLinear;
		Ref<Image> m_DummyWhiteTexture;

		Ref<DeviceBuffer> m_CameraDataBuffer;

		Ptr<VirtualMemoryBlock> m_TextureIndexAllocator;
		Ptr<VirtualMemoryBlock> m_StorageImageIndexAllocator;
		rhumap<UUID, uint32> m_TextureIndices;
		rhumap<UUID, uint32> m_StorageImageIndices;

		DeviceIndexedResourceBuffer<GeometryMeshData> m_MeshResourcesBuffer;
		DeviceMaterialPool m_MaterialDataPool;

		Ref<DeviceBuffer> m_DeviceRenderQueue;
		std::vector<HostInstanceRenderData> m_HighLevelInstanceQueue;
		std::vector<InstanceRenderData> m_HostRenderQueue;
		std::vector<Sprite> m_SpriteQueue;

		std::vector<PointLight> m_HostPointLights;
		std::shared_mutex m_Mutex;

		DebugSceneView m_CurrentViewMode = DebugSceneView::NONE;

	};

}