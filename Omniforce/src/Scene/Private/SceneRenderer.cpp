#include "../SceneRenderer.h"

#include <Renderer/DescriptorSet.h>
#include <Renderer/Pipeline.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/DeviceBuffer.h>
#include <Core/Utils.h>

namespace Omni {

	SceneRenderer* SceneRenderer::Create(const SceneRendererSpecification& spec)
	{
		return new SceneRenderer(spec);
	}

	SceneRenderer::SceneRenderer(const SceneRendererSpecification& spec)
		: m_Specification(spec)
	{
		{
			std::vector<DescriptorBinding> bindings;
			// Camera Data
			bindings.push_back({ 0, DescriptorBindingType::SAMPLED_IMAGE, s_GlobalSceneData.max_textures, (uint64)DescriptorFlags::PARTIALLY_BOUND });
			bindings.push_back({ 1, DescriptorBindingType::UNIFORM_BUFFER, 1, 0 });
			bindings.push_back({ 2, DescriptorBindingType::STORAGE_BUFFER, 1, 0 });

			DescriptorSetSpecification global_set_spec = {};
			global_set_spec.bindings = std::move(bindings);

			for (int i = 0; i < 3; i++) {
				auto set = DescriptorSet::Create(global_set_spec);
				s_GlobalSceneData.scene_descriptor_set.push_back(set);
			}

			s_GlobalSceneData.available_texture_indices.reserve(s_GlobalSceneData.max_textures);
			s_GlobalSceneData.available_texture_indices.resize(s_GlobalSceneData.max_textures);
			uint32 index = 0;
			for (auto i = s_GlobalSceneData.available_texture_indices.rbegin(); i != s_GlobalSceneData.available_texture_indices.rend(); i++, index++)
				*i = index;

			// Create camera data buffer
			{
				uint32 per_frame_size = Utils::Align(sizeof(glm::mat4) * 3, Renderer::GetDeviceMinimalUniformBufferOffsetAlignment());

				DeviceBufferSpecification buffer_spec = {};
				buffer_spec.size = per_frame_size * Renderer::GetConfig().frames_in_flight;
				buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
				buffer_spec.buffer_usage = DeviceBufferUsage::UNIFORM_BUFFER;

				m_MainCameraDataBuffer = DeviceBuffer::Create(buffer_spec);

				for (uint32 i = 0; i < Renderer::GetConfig().frames_in_flight; i++) {
					s_GlobalSceneData.scene_descriptor_set[i]->Write(1, 0, m_MainCameraDataBuffer, per_frame_size, i * per_frame_size);
				}
			}

			// Create sprite data buffer. Creating SSBO because I need more than 65kb.
			{
				uint32 per_frame_size = Utils::Align(sizeof(Sprite) * 2500, Renderer::GetDeviceMinimalStorageBufferOffsetAlignment());

				DeviceBufferSpecification buffer_spec = {};
				buffer_spec.size = per_frame_size * Renderer::GetConfig().frames_in_flight;
				buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
				buffer_spec.buffer_usage = DeviceBufferUsage::STORAGE_BUFFER;

				m_SpriteDataBuffer = DeviceBuffer::Create(buffer_spec);

				for (uint32 i = 0; i < Renderer::GetConfig().frames_in_flight; i++) {
					s_GlobalSceneData.scene_descriptor_set[i]->Write(2, 0, m_SpriteDataBuffer, per_frame_size, i * per_frame_size);
				}

				m_SpriteBufferSize = per_frame_size;
			}
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
			sampler_spec.anisotropic_filtering_level = m_Specification.anisotropic_filtering;

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
			sampler_spec.anisotropic_filtering_level = m_Specification.anisotropic_filtering;

			m_SamplerLinear = ImageSampler::Create(sampler_spec);
		}
		// Load dummy white texture
		{
			ImageSpecification image_spec = {};
			image_spec.path = "assets/textures/dummy_white_texture.png";
			m_DummyWhiteTexture = Image::Create(image_spec);
			AcquireTextureIndex(m_DummyWhiteTexture, SamplerFilteringMode::LINEAR);
		}
		// Initializing pipelines
		{
			PipelineSpecification pipeline_spec = PipelineSpecification::Default();
			pipeline_spec.shader = ShaderLibrary::Get()->GetShader("sprite.ofs");
			pipeline_spec.input_layout = DeviceBufferLayout({});
			pipeline_spec.debug_name = "sprite pass";
			pipeline_spec.output_attachments_formats = { ImageFormat::BGRA32 };

			m_SpritePass = Pipeline::Create(pipeline_spec);
			ShaderLibrary::Get()->Unload("sprite.ofs");
		}
		
	}

	SceneRenderer::~SceneRenderer()
	{

	}

	void SceneRenderer::Destroy()
	{
		m_SamplerLinear->Destroy();
		m_SamplerNearest->Destroy();
		m_DummyWhiteTexture->Destroy();
		m_SpritePass->Destroy();
		m_SpriteDataBuffer->Destroy();
		m_MainCameraDataBuffer->Destroy();
		for(auto set : s_GlobalSceneData.scene_descriptor_set)
			set->Destroy();
	}

	void SceneRenderer::BeginScene(SceneRenderData scene_data)
	{
		m_CurrentSceneRenderData = scene_data;

		glm::mat4 matrices[3] = { scene_data.camera->GetViewMatrix(), scene_data.camera->GetProjectionMatrix(), scene_data.camera->GetViewProjectionMatrix() };

		m_MainCameraDataBuffer->UploadData(Renderer::GetCurrentFrameIndex() * (sizeof glm::mat4 * 3), matrices, sizeof(matrices));

		Renderer::BeginRender(scene_data.target, scene_data.target->GetSpecification().extent, { 0, 0 }, { 0.0f, 0.0f, 0.0f, 1.0f });
		Renderer::BindSet(s_GlobalSceneData.scene_descriptor_set[Renderer::GetCurrentFrameIndex()], m_SpritePass, 0);
	}

	void SceneRenderer::EndScene()
	{
		m_SpriteDataBuffer->UploadData(
			Renderer::GetCurrentFrameIndex() * m_SpriteBufferSize,
			m_SpriteQueue.data(),
			m_SpriteQueue.size() * sizeof(Sprite)
		);

		Renderer::RenderQuad(m_SpritePass, m_SpriteQueue.size(), { nullptr, 0 });
		Renderer::EndRender(m_CurrentSceneRenderData.target);

		m_SpriteQueue.clear();
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
		case SamplerFilteringMode::NEAREST:		sampler = m_SamplerNearest; break;
		case SamplerFilteringMode::LINEAR:		sampler = m_SamplerLinear;  break;
		default:								OMNIFORCE_ASSERT_TAGGED(false, "Invalid sampler filtering mode"); break;
		}

		for(auto& set : s_GlobalSceneData.scene_descriptor_set)
			set->Write(0, index, image, sampler);
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
		OMNIFORCE_ASSERT_TAGGED(false, "Currently not implemented properly. Basic color pass is not inited.");
		MiscData data;
		data.size = sizeof(s_GlobalSceneData.textures[texture->GetId()]);
		data.data = new uint8[data.size];

		memcpy(data.data, &s_GlobalSceneData.textures[texture->GetId()], sizeof(uint32));

		Renderer::BindSet(s_GlobalSceneData.scene_descriptor_set[Renderer::GetCurrentFrameIndex()], m_BasicColorPass, 0);
		Renderer::RenderMesh(m_BasicColorPass, vbo, ibo, data);
	}

	void SceneRenderer::RenderSprite(const Sprite& sprite)
	{
		m_SpriteQueue.push_back(sprite);
	}

}