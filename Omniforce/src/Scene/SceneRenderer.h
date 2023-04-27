#pragma once

#include "SceneCommon.h"
#include <Renderer/Renderer.h>
#include <Renderer/Image.h>

#include <robin_hood.h>
#include <vector>

#include <glm/glm.hpp>

namespace Omni {

	struct CameraData {
		glm::mat4 view_matrix;
		glm::mat4 view_projection_matrix;
		glm::mat4 projection_matrix;
	};

	struct OMNIFORCE_API SceneRenderData {
		Shared<Image> target;
		CameraData camera_data;
	};

	struct OMNIFORCE_API SceneRendererSpecification {
		uint8 anisotropic_filtering = 16;
	};

	class OMNIFORCE_API SceneRenderer {
	public:
		static SceneRenderer* Create(const SceneRendererSpecification& spec);

		SceneRenderer(const SceneRendererSpecification& spec);
		~SceneRenderer();

		void Destroy();

		void BeginScene(SceneRenderData scene_data);
		void EndScene();
		
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

		void RenderMesh(Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo, Shared<Image> texture);

	private:
		SceneRenderData m_CurrentSceneRenderData;
		SceneRendererSpecification m_Specification;

		Shared<DeviceBuffer> m_CameraData;
		Shared<ImageSampler> m_SamplerNearest;
		Shared<ImageSampler> m_SamplerLinear;

		Shared<Pipeline> m_BasicColorPass;

		struct GlobalSceneRenderData {
			const uint32 max_textures = UINT16_MAX + 1;
			robin_hood::unordered_map<uint32, uint32> textures;
			std::vector<uint32> available_texture_indices;
			std::vector<Shared<DescriptorSet>> scene_descriptor_set;
		} s_GlobalSceneData;

	};

}