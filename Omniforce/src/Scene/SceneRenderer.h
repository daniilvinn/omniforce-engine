#pragma once

#include "SceneCommon.h"
#include "Sprite.h"
#include "Camera.h"
#include <Renderer/Renderer.h>
#include <Renderer/Image.h>

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
		UUID GetDummyWhiteTexture() const { return m_DummyWhiteTexture->GetId(); }

		/*
		* @brief Adds texture to a global renderer data
		* @return returns an index the texture can be accessed with
		*/
		uint16 AcquireTextureIndex(Shared<Image> image, SamplerFilteringMode filtering_mode);

		/*
		* @brief Removes texture to a global renderer data
		* @return true if successful, false if no texture found
		*/
		bool ReleaseTextureIndex(Shared<Image> image);

		uint32 GetTextureIndex(const UUID& uuid) const { return s_GlobalSceneData.textures.at(uuid); };

		void RenderMesh(Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo, Shared<Image> texture);

		void RenderSprite(const Sprite& sprite);

		void Copy(SceneRenderer* other);

	private:
		Shared<Camera> m_Camera;
		SceneRendererSpecification m_Specification;

		std::vector<Shared<Image>> m_RendererOutputs;
		Shared<Image> m_CurrectMainRenderTarget;

		inline static Shared<ImageSampler> s_SamplerNearest;
		inline static Shared<ImageSampler> s_SamplerLinear;
		Shared<Image> m_DummyWhiteTexture;
		Shared<DeviceBuffer> m_MainCameraDataBuffer;
		Shared<DeviceBuffer> m_SpriteDataBuffer;
		uint32 m_SpriteBufferSize; // size in bytes per frame in flight, not overall size
		std::vector<Sprite> m_SpriteQueue;

		Shared<Pipeline> m_BasicColorPass;
		Shared<Pipeline> m_SpritePass;

		struct GlobalSceneRenderData {
			const uint32 max_textures = UINT16_MAX + 1;
			robin_hood::unordered_map<UUID, uint32> textures;
			std::vector<uint32> available_texture_indices;
			std::vector<Shared<DescriptorSet>> scene_descriptor_set; // per frame in flight
		} s_GlobalSceneData;

	};

}