#include "../SceneRenderer.h"

#include <Renderer/DescriptorSet.h>
#include <Renderer/Pipeline.h>
#include <Renderer/ShaderLibrary.h>

namespace Omni {

	SceneRenderer::SceneRenderer(const SceneRendererSpecification& spec)
		: m_Specification(spec)
	{
		{
			std::vector<DescriptorBinding> bindings;
			// Camera Data
			bindings.push_back({ 0, DescriptorBindingType::SAMPLED_IMAGE, s_GlobalSceneData.max_textures, (uint64)DescriptorFlags::PARTIALLY_BOUND });

			DescriptorSetSpecification global_set_spec = {};
			global_set_spec.bindings = std::move(bindings);

			s_GlobalSceneData.scene_descriptor_set = DescriptorSet::Create(global_set_spec);

			s_GlobalSceneData.available_texture_indices.reserve(s_GlobalSceneData.max_textures);
			s_GlobalSceneData.available_texture_indices.resize(s_GlobalSceneData.max_textures);
			uint32 index = 0;
			for (auto i = s_GlobalSceneData.available_texture_indices.rbegin(); i != s_GlobalSceneData.available_texture_indices.rend(); i++, index++)
				*i = index;

			s_GlobalSceneData.scene_descriptor_set->Write(0, 1, m_CameraData, sizeof CameraData, 0);
		}
		
		// Initializing nearest filtration sampler
		{
			ImageSamplerSpecification sampler_spec = {};
			sampler_spec.min_filtering_mode = SamplerFilteringMode::NEAREST;
			sampler_spec.mag_filtering_mode = SamplerFilteringMode::NEAREST;
			sampler_spec.mipmap_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.address_mode = SamplerAddressMode::CLAMP;
			sampler_spec.min_lod = 0.0f;
			sampler_spec.max_lod = 1000.0f;
			sampler_spec.lod_bias = 0.0f;
			sampler_spec.anisotropy_filtering_level = m_Specification.anisotropy_filtration;

			m_SamplerNearest = ImageSampler::Create(sampler_spec);
		}
		// Initializing linear filtration sampler
		{
			ImageSamplerSpecification sampler_spec = {};
			sampler_spec.min_filtering_mode = SamplerFilteringMode::NEAREST;
			sampler_spec.mag_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.mipmap_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.address_mode = SamplerAddressMode::CLAMP;
			sampler_spec.min_lod = 0.0f;
			sampler_spec.max_lod = 1000.0f;
			sampler_spec.lod_bias = 0.0f;
			sampler_spec.anisotropy_filtering_level = m_Specification.anisotropy_filtration;

			m_SamplerLinear = ImageSampler::Create(sampler_spec);
		}
		// Initializing pipelines
		
	}

	SceneRenderer::~SceneRenderer()
	{

	}

	void SceneRenderer::BeginScene(const SceneRenderData& scene_data)
	{
		m_CurrentSceneRenderData = scene_data;
		Renderer::BeginRender(scene_data.target, scene_data.target->GetSpecification().extent, { 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f });
	}

	void SceneRenderer::EndScene()
	{
		Renderer::EndRender(m_CurrentSceneRenderData.target);
	}

	uint16 SceneRenderer::AcquireTextureIndex(Shared<Image> image, SamplerFilteringMode filtering_mode)
	{
		if (s_GlobalSceneData.available_texture_indices.size() == 0) {
			OMNIFORCE_CORE_ERROR("Exceeded maximum amount of textures!");
			return UINT16_MAX;
		}

		uint16 index = s_GlobalSceneData.available_texture_indices.back();
		s_GlobalSceneData.available_texture_indices.pop_back();

		Shared<ImageSampler> sampler;

		switch (filtering_mode)
		{
		case SamplerFilteringMode::NEAREST: sampler = m_SamplerNearest; break;
		case SamplerFilteringMode::LINEAR:	sampler = m_SamplerLinear; break;
		default:							OMNIFORCE_ASSERT_TAGGED(false, "Invalid sampler filtering mode"); break;
		}

		s_GlobalSceneData.scene_descriptor_set->Write(1, index, image, sampler);
		s_GlobalSceneData.textures.emplace(image->GetId(), index);

		return index;
	}

	bool SceneRenderer::ReleaseTextureIndex(Shared<Image> image)
	{
		if (s_GlobalSceneData.available_texture_indices.size() == s_GlobalSceneData.max_textures) {
			OMNIFORCE_CORE_ERROR("Texture index bank is full!");
			return false;
		}

		uint16 index = s_GlobalSceneData.textures.find(image->GetId())->second;

		s_GlobalSceneData.available_texture_indices.push_back(index);
		s_GlobalSceneData.textures.erase(image->GetId());

		return true;
	}

	void SceneRenderer::RenderMesh(Shared<DeviceBuffer> vbo, Shared<DeviceBuffer> ibo, Shared<Image> texture)
	{

	}

}