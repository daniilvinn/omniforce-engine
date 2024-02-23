#pragma once

#include "SceneCommon.h"
#include "Sprite.h"
#include "Camera.h"
#include "DeviceMaterialPool.h"
#include "SceneRendererPrimitives.h"
#include "DeviceIndexedResourceBuffer.h"
#include <Renderer/Renderer.h>
#include <Renderer/Image.h>
#include <Renderer/Mesh.h>
#include <Core/CallbackRHUMap.h>
#include <Memory/VirtualMemoryBlock.h>

#include <vector>

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
		
		Shared<Image> GetFinalImage();
		static Shared<ImageSampler> GetSamplerNearest() { return s_SamplerNearest; }
		static Shared<ImageSampler> GetSamplerLinear() { return s_SamplerLinear; }
		AssetHandle GetDummyWhiteTexture() const { return m_DummyWhiteTexture->Handle; }
		/*
		* @brief Adds resource to a global renderer data
		* @return returns an index the resource can be accessed with
		*/
		uint32 AcquireResourceIndex(Shared<Image> image, SamplerFilteringMode filtering_mode);
		uint32 AcquireResourceIndex(Shared<Mesh> mesh);
		uint32 AcquireResourceIndex(Shared<Material> material); // WARNING: this returns offset in material pool, not index

		/*
		* @brief Removes resource to a global renderer data
		* @return true if successful, false if no resource found
		*/
		bool ReleaseResourceIndex(Shared<Image> image);
		uint32 GetTextureIndex(const AssetHandle& uuid) const { return m_TextureIndices.at(uuid); };
		uint64 GetMaterialBDA(const AssetHandle& id) const { return m_MaterialDataPool.GetStorageBufferAddress() + m_MaterialDataPool.GetOffset(id); }
		uint32 GetMeshIndex(const AssetHandle& uuid) const { return m_MeshResourcesBuffer.GetIndex(uuid); }

		void RenderSprite(const Sprite& sprite);
		void RenderObject(Shared<Pipeline> pipeline, const DeviceRenderableObject& render_data);

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


		// ~ Omni 2024 ~
		Shared<DeviceBuffer> m_CameraDataBuffer;

		Scope<VirtualMemoryBlock> m_TextureIndexAllocator;
		rhumap<UUID, uint32> m_TextureIndices;
		Shared<DeviceBuffer> m_SpriteDataBuffer;

		DeviceIndexedResourceBuffer<DeviceMeshData> m_MeshResourcesBuffer;
		DeviceMaterialPool m_MaterialDataPool;

		rhumap<Shared<Pipeline>, std::vector<DeviceRenderableObject>> m_HostRenderQueue;
		CallbackRHUMap<Shared<Pipeline>, Shared<DeviceBuffer>> m_DeviceRenderQueue;
		rhumap<Shared<Pipeline>, Shared<DeviceBuffer>> m_CulledDeviceRenderQueue;
		rhumap<Shared<Pipeline>, Shared<DeviceBuffer>> m_DeviceIndirectDrawParams;
		 
		Shared<Pipeline> m_IndirectFrustumCullPipeline;

	};

}