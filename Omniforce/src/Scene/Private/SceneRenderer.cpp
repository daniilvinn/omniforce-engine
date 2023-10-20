#include "../SceneRenderer.h"

#include <Renderer/DescriptorSet.h>
#include <Renderer/Pipeline.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/DeviceBuffer.h>
#include <Core/Utils.h>

#include <Asset/AssetManager.h>
#include <Asset/AssetCompressor.h>
#include <Platform/Vulkan/VulkanCommon.h>

namespace Omni {

	Shared<SceneRenderer> SceneRenderer::Create(const SceneRendererSpecification& spec)
	{
		return std::make_shared<SceneRenderer>(spec);
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

			for (int i = 0; i < Renderer::GetConfig().frames_in_flight; i++) {
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

			// Initialize render targets
			{
				ImageSpecification render_target_spec = ImageSpecification::Default();
				render_target_spec.usage = ImageUsage::RENDER_TARGET;
				render_target_spec.extent = Renderer::GetSwapchainImage()->GetSpecification().extent;
				render_target_spec.format = ImageFormat::RGB32_HDR;
				for (int i = 0; i < Renderer::GetConfig().frames_in_flight; i++)
					m_RendererOutputs.push_back(Image::Create(render_target_spec));
			}

		}

		// Initializing nearest filtration sampler
		{
			ImageSamplerSpecification sampler_spec = {};
			sampler_spec.min_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.mag_filtering_mode = SamplerFilteringMode::NEAREST;
			sampler_spec.mipmap_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.address_mode = SamplerAddressMode::CLAMP;
			sampler_spec.min_lod = 0.0f;
			sampler_spec.max_lod = 1000.0f;
			sampler_spec.lod_bias = 0.0f;
			sampler_spec.anisotropic_filtering_level = m_Specification.anisotropic_filtering;

			s_SamplerNearest = ImageSampler::Create(sampler_spec);
		}
		// Initializing linear filtration sampler
		{
			ImageSamplerSpecification sampler_spec = {};
			sampler_spec.min_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.mag_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.mipmap_filtering_mode = SamplerFilteringMode::LINEAR;
			sampler_spec.address_mode = SamplerAddressMode::CLAMP;
			sampler_spec.min_lod = 0.0f;
			sampler_spec.max_lod = 1000.0f;
			sampler_spec.lod_bias = 0.0f;
			sampler_spec.anisotropic_filtering_level = m_Specification.anisotropic_filtering;

			s_SamplerLinear = ImageSampler::Create(sampler_spec);
		}
		// Load dummy white texture
		{
			std::vector<RGBA32> image_data(16);
			std::memset(image_data.data(), 255, 64); // just set every byte to 255, so we have white non-transparent pixels
			std::vector<byte> bc7_compressed = AssetCompressor::CompressBC7(image_data, 4, 4);

			ImageSpecification image_spec = ImageSpecification::Default();
			image_spec.usage = ImageUsage::TEXTURE;
			image_spec.extent = { 4, 4, 1 };
			image_spec.pixels = std::move(bc7_compressed);
			image_spec.format = ImageFormat::RGBA32_UNORM;
			image_spec.type = ImageType::TYPE_2D;
			image_spec.mip_levels = 1;
			image_spec.array_layers = 1;

			m_DummyWhiteTexture = Image::Create(image_spec);
			AcquireTextureIndex(m_DummyWhiteTexture, SamplerFilteringMode::LINEAR);
		}
		// Initializing pipelines
		{
			PipelineSpecification pipeline_spec = PipelineSpecification::Default();
			pipeline_spec.shader = ShaderLibrary::Get()->GetShader("sprite.ofs");
			pipeline_spec.debug_name = "sprite pass";
			pipeline_spec.output_attachments_formats = { ImageFormat::RGB32_HDR };
			pipeline_spec.culling_mode = PipelineCullingMode::NONE;

			m_SpritePass = Pipeline::Create(pipeline_spec);
		}

	}

	SceneRenderer::~SceneRenderer()
	{

	}

	void SceneRenderer::Destroy()
	{
		s_SamplerLinear->Destroy();
		s_SamplerNearest->Destroy();
		m_DummyWhiteTexture->Destroy();
		m_SpritePass->Destroy();
		m_SpriteDataBuffer->Destroy();
		m_MainCameraDataBuffer->Destroy();
		for (auto set : s_GlobalSceneData.scene_descriptor_set)
			set->Destroy();
		for (auto& output : m_RendererOutputs)
			output->Destroy();
	}

	void SceneRenderer::BeginScene(Shared<Camera> camera)
	{
		if (camera) {
			m_Camera = camera;
			glm::mat4 matrices[3] = { m_Camera->GetViewMatrix(), m_Camera->GetProjectionMatrix(), m_Camera->GetViewProjectionMatrix() };
			m_MainCameraDataBuffer->UploadData(Renderer::GetCurrentFrameIndex() * (sizeof glm::mat4 * 3), matrices, sizeof(matrices)); // TODO: bug here
		}

		m_CurrectMainRenderTarget = m_RendererOutputs[Renderer::GetCurrentFrameIndex()];

		Renderer::Submit([=]() {	
			m_CurrectMainRenderTarget->SetLayout(
				Renderer::GetCmdBuffer(),
				ImageLayout::COLOR_ATTACHMENT,
				PipelineStage::FRAGMENT_SHADER,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				PipelineAccess::UNIFORM_READ,
				PipelineAccess::COLOR_ATTACHMENT_WRITE
			);
		});
		Renderer::BeginRender(m_CurrectMainRenderTarget, m_CurrectMainRenderTarget->GetSpecification().extent, { 0, 0 }, { 0.0f, 0.0f, 0.0f, 1.0f });
		Renderer::BindSet(s_GlobalSceneData.scene_descriptor_set[Renderer::GetCurrentFrameIndex()], m_SpritePass, 0);
	}

	void SceneRenderer::EndScene()
	{
		// Draw 2D
		m_SpriteDataBuffer->UploadData(
			Renderer::GetCurrentFrameIndex() * m_SpriteBufferSize,
			m_SpriteQueue.data(),
			m_SpriteQueue.size() * sizeof(Sprite)
		);

		Renderer::RenderQuads(m_SpritePass, m_SpriteQueue.size(), { nullptr, 0 });
		Renderer::EndRender(m_CurrectMainRenderTarget);
		Renderer::Submit([=]() {
			m_CurrectMainRenderTarget->SetLayout(
				Renderer::GetCmdBuffer(),
				ImageLayout::SHADER_READ_ONLY,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				PipelineStage::FRAGMENT_SHADER,
				PipelineAccess::COLOR_ATTACHMENT_WRITE,
				PipelineAccess::UNIFORM_READ
			);
		});
		
		m_SpriteQueue.clear();

		// Draw 3D
	}

	Shared<Image> SceneRenderer::GetFinalImage()
	{
		return m_RendererOutputs[Renderer::GetCurrentFrameIndex()];
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
		case SamplerFilteringMode::NEAREST:		sampler = s_SamplerNearest; break;
		case SamplerFilteringMode::LINEAR:		sampler = s_SamplerLinear;  break;
		default:								OMNIFORCE_ASSERT_TAGGED(false, "Invalid sampler filtering mode"); break;
		}

		for (auto& set : s_GlobalSceneData.scene_descriptor_set)
			set->Write(0, index, image, sampler);
		s_GlobalSceneData.textures.emplace(image->Handle, index);

		return index;
	}

	bool SceneRenderer::ReleaseTextureIndex(Shared<Image> image)
	{
		if (s_GlobalSceneData.available_texture_indices.size() == s_GlobalSceneData.max_textures) {
			OMNIFORCE_CORE_ERROR("Texture index bank is full!");
			return false;
		}

		uint16 index = s_GlobalSceneData.textures.find(image->Handle)->second;

		s_GlobalSceneData.available_texture_indices.push_back(index);
		s_GlobalSceneData.textures.erase(image->Handle);

		return true;
	}

	void SceneRenderer::RenderSprite(const Sprite& sprite)
	{
		m_SpriteQueue.push_back(sprite);
	}

}