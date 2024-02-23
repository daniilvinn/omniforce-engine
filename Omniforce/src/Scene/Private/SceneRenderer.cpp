#include "../SceneRenderer.h"

#include <Renderer/DescriptorSet.h>
#include <Renderer/Pipeline.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/PipelineBarrier.h>
#include <Core/Utils.h>
#include "../SceneRendererPrimitives.h"

#include <Asset/AssetManager.h>
#include <Asset/AssetCompressor.h>
#include <Platform/Vulkan/VulkanCommon.h>

namespace Omni {

	Shared<SceneRenderer> SceneRenderer::Create(const SceneRendererSpecification& spec)
	{
		return std::make_shared<SceneRenderer>(spec);
	}

	SceneRenderer::SceneRenderer(const SceneRendererSpecification& spec)
		: m_Specification(spec), m_MaterialDataPool(this, 1024 * 256 /* 256Kb of data*/), m_TextureIndexAllocator(VirtualMemoryBlock::Create(4 * INT16_MAX)),
		m_MeshResourcesBuffer(1024 * sizeof DeviceMeshData) // allow up to 1024 meshes be allocated at once
	{
		AssetManager* asset_manager = AssetManager::Get();

		// Descriptor data
		{
			std::vector<DescriptorBinding> bindings;
			// Camera Data
			bindings.push_back({ 0, DescriptorBindingType::SAMPLED_IMAGE, UINT16_MAX, (uint64)DescriptorFlags::PARTIALLY_BOUND });
			bindings.push_back({ 1, DescriptorBindingType::STORAGE_BUFFER, 1, 0 });

			DescriptorSetSpecification global_set_spec = {};
			global_set_spec.bindings = std::move(bindings);

			for (int i = 0; i < Renderer::GetConfig().frames_in_flight; i++) {
				auto set = DescriptorSet::Create(global_set_spec);
				m_SceneDescriptorSet.push_back(set);
			}

			// Create sprite data buffer. Creating SSBO because I need more than 65kb.
			{
				uint32 per_frame_size = Utils::Align(sizeof(Sprite) * 2500, Renderer::GetDeviceMinimalStorageBufferOffsetAlignment());

				DeviceBufferSpecification buffer_spec = {};
				buffer_spec.size = per_frame_size * Renderer::GetConfig().frames_in_flight;
				buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
				buffer_spec.buffer_usage = DeviceBufferUsage::STORAGE_BUFFER;
				buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;

				m_SpriteDataBuffer = DeviceBuffer::Create(buffer_spec);

				for (uint32 i = 0; i < Renderer::GetConfig().frames_in_flight; i++) {
					m_SceneDescriptorSet[i]->Write(1, 0, m_SpriteDataBuffer, per_frame_size, i * per_frame_size);
				}

				m_SpriteBufferSize = per_frame_size;
			}

			// Initialize render targets
			{
				ImageSpecification attachment_spec = ImageSpecification::Default();
				attachment_spec.usage = ImageUsage::RENDER_TARGET;
				attachment_spec.extent = Renderer::GetSwapchainImage()->GetSpecification().extent;
				attachment_spec.format = ImageFormat::RGB32_HDR;
				for (int i = 0; i < Renderer::GetConfig().frames_in_flight; i++)
					m_RendererOutputs.push_back(Image::Create(attachment_spec));

				attachment_spec.usage = ImageUsage::DEPTH_BUFFER;
				attachment_spec.format = ImageFormat::D32;

				for (int i = 0; i < Renderer::GetConfig().frames_in_flight; i++)
					m_DepthAttachments.push_back(Image::Create(attachment_spec));

			}

		}

		{
			// Initialize camera buffer
			DeviceBufferSpecification buffer_spec = {};
			buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
			buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
			buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
			buffer_spec.size = sizeof DeviceCameraData * Renderer::GetConfig().frames_in_flight;
			m_CameraDataBuffer = DeviceBuffer::Create(buffer_spec);
		}

		// Initialize nearest filtration sampler
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
			RGBA32 image_data(255, 255, 255, 255);

			ImageSpecification image_spec = ImageSpecification::Default();
			image_spec.usage = ImageUsage::TEXTURE;
			image_spec.extent = { 1, 1, 1 };
			image_spec.pixels = std::vector<byte>((byte*)&image_data, (byte*) ( & image_data + 1));
			image_spec.format = ImageFormat::RGBA32_UNORM;
			image_spec.type = ImageType::TYPE_2D;
			image_spec.mip_levels = 1;
			image_spec.array_layers = 1;

			m_DummyWhiteTexture = Image::Create(image_spec, 0);
			AcquireResourceIndex(m_DummyWhiteTexture, SamplerFilteringMode::LINEAR);
		}
		// Initializing pipelines
		{
			ShaderLibrary* shader_library = ShaderLibrary::Get();
			shader_library->LoadShader("Resources/shaders/sprite.ofs");
			shader_library->LoadShader("Resources/shaders/cull_indirect.ofs");

			// 2D pass
			PipelineSpecification pipeline_spec = PipelineSpecification::Default();
			pipeline_spec.shader = shader_library->GetShader("sprite.ofs");
			pipeline_spec.debug_name = "sprite pass";
			pipeline_spec.output_attachments_formats = { ImageFormat::RGB32_HDR };
			pipeline_spec.culling_mode = PipelineCullingMode::NONE;

			m_SpritePass = Pipeline::Create(pipeline_spec);

			// compute frustum culling
			pipeline_spec.type = PipelineType::COMPUTE;
			pipeline_spec.shader = shader_library->GetShader("cull_indirect.ofs");
			pipeline_spec.debug_name = "frustum cull prepass";

			m_IndirectFrustumCullPipeline = Pipeline::Create(pipeline_spec);
		}
		// Initialize device render queue and all buffers for indirect drawing
		{
			m_DeviceRenderQueue.add_callback([&](const Shared<Pipeline>& pipeline, Shared<DeviceBuffer>& value) {
				DeviceBufferSpecification spec;
				// allow for 256 objects per material for beginning.
				// TODO: allow recreating buffer with bigger size when limit is reached
				spec.size = sizeof(DeviceRenderableObject) * 256 * Renderer::GetConfig().frames_in_flight; 
				spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
				spec.heap =			DeviceBufferMemoryHeap::DEVICE;
				spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
				
				value = DeviceBuffer::Create(spec);

				spec.size = sizeof(DeviceRenderableObject) * 256;
				spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
				
				m_CulledDeviceRenderQueue.emplace(pipeline, DeviceBuffer::Create(spec));

				spec.size = 4 + sizeof(glm::uvec3) * 256;
				spec.buffer_usage = DeviceBufferUsage::INDIRECT_PARAMS;
				m_DeviceIndirectDrawParams.emplace(pipeline, DeviceBuffer::Create(spec));
			});
		}
	}

	SceneRenderer::~SceneRenderer()
	{

	}

	void SceneRenderer::Destroy()
	{
		s_SamplerLinear->Destroy();
		s_SamplerNearest->Destroy();
		//m_DummyWhiteTexture->Destroy();
		m_SpritePass->Destroy();
		m_SpriteDataBuffer->Destroy();
		m_CameraDataBuffer->Destroy();
		for (auto set : m_SceneDescriptorSet)
			set->Destroy();
		for (auto& output : m_RendererOutputs)
			output->Destroy();
	}

	void SceneRenderer::BeginScene(Shared<Camera> camera)
	{
		// Clear render queues
		for (auto& queue : m_HostRenderQueue)
			queue.second.clear();
		
		m_SpriteQueue.clear();

		// Write camera data to device buffer
		if (camera) {
			m_Camera = camera;

			DeviceCameraData camera_data;
			camera_data.view = m_Camera->GetViewMatrix();
			camera_data.proj = m_Camera->GetProjectionMatrix();
			camera_data.view_proj = m_Camera->GetViewProjectionMatrix();
			camera_data.position = m_Camera->GetPosition();
			camera_data.frustum = m_Camera->GenerateFrustum();

			m_CameraDataBuffer->UploadData(
				Renderer::GetCurrentFrameIndex() * (m_CameraDataBuffer->GetSpecification().size / Renderer::GetConfig().frames_in_flight),
				&camera_data,
				sizeof camera_data
			);
		}

		// Change current render target
		m_CurrectMainRenderTarget = m_RendererOutputs[Renderer::GetCurrentFrameIndex()];
		m_CurrentDepthAttachment  = m_DepthAttachments[Renderer::GetCurrentFrameIndex()];

		// Change layout of render target
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

	}

	void SceneRenderer::EndScene()
	{
		uint64 camera_data_device_address = m_CameraDataBuffer->GetDeviceAddress() + sizeof DeviceCameraData * Renderer::GetCurrentFrameIndex();

		// Render 2D
		{
			Renderer::BeginRender({ m_CurrectMainRenderTarget, m_CurrentDepthAttachment }, m_CurrectMainRenderTarget->GetSpecification().extent, { 0, 0 }, { 0.2f, 0.2f, 0.3f, 1.0f });
			m_SpriteDataBuffer->UploadData(
				Renderer::GetCurrentFrameIndex() * m_SpriteBufferSize,
				m_SpriteQueue.data(),
				m_SpriteQueue.size() * sizeof(Sprite)
			);

			MiscData pc = {};
			pc.data = (byte*)new uint64;
			memcpy(pc.data, &camera_data_device_address, sizeof uint64);
			pc.size = sizeof uint64;

			Renderer::BindSet(m_SceneDescriptorSet[Renderer::GetCurrentFrameIndex()], m_SpritePass, 0);
			Renderer::RenderQuads(m_SpritePass, m_SpriteQueue.size(), pc);
			Renderer::EndRender({ m_CurrectMainRenderTarget });
		}

		// Copy data to render queues
		for (auto& host_render_queue : m_HostRenderQueue) {
			auto device_render_queue = m_DeviceRenderQueue[host_render_queue.first];
			uint32 per_frame_size = device_render_queue->GetSpecification().size / Renderer::GetConfig().frames_in_flight;
			device_render_queue->UploadData(
				Renderer::GetCurrentFrameIndex() * per_frame_size,
				host_render_queue.second.data(),
				host_render_queue.second.size() * sizeof DeviceRenderableObject
			);
		}

		// Cull 3D
		std::vector<std::pair<Shared<DeviceBuffer>, PipelineResourceBarrierInfo>> culling_buffers_barriers;
		for (auto& device_render_queue : m_DeviceRenderQueue) {
			uint32 device_queue_per_frame_size = device_render_queue.second->GetSpecification().size / Renderer::GetConfig().frames_in_flight;

			Shared<DeviceBuffer> culled_objects_buffer = m_CulledDeviceRenderQueue[device_render_queue.first];
			Shared<DeviceBuffer> indirect_params_buffer = m_DeviceIndirectDrawParams[device_render_queue.first];

			IndirectFrustumCullPassPushContants* compute_push_constants_data = new IndirectFrustumCullPassPushContants;
			compute_push_constants_data->camera_data_bda = camera_data_device_address;
			compute_push_constants_data->mesh_data_bda = m_MeshResourcesBuffer.GetStorageBDA();
			compute_push_constants_data->render_objects_data_bda = device_render_queue.second->GetDeviceAddress() + Renderer::GetCurrentFrameIndex() * device_queue_per_frame_size;
			compute_push_constants_data->original_object_count = m_HostRenderQueue[device_render_queue.first].size();
			compute_push_constants_data->culled_objects_bda = culled_objects_buffer->GetDeviceAddress();
			compute_push_constants_data->object_counter_bda = indirect_params_buffer->GetDeviceAddress();
			compute_push_constants_data->indirect_draw_params_bda = indirect_params_buffer->GetDeviceAddress() + 4;

			MiscData compute_pc = {};
			compute_pc.data = (byte*)compute_push_constants_data;
			compute_pc.size = sizeof IndirectFrustumCullPassPushContants;

			uint32 num_work_groups = (uint32)glm::round(((float32)m_HostRenderQueue[device_render_queue.first].size() / 32.0f) + 0.4999999f);
			Renderer::DispatchCompute(m_IndirectFrustumCullPipeline, { num_work_groups, 1, 1 }, compute_pc);

			PipelineResourceBarrierInfo indirect_params_barrier = {};
			indirect_params_barrier.src_stages = (BitMask)PipelineStage::COMPUTE_SHADER;
			indirect_params_barrier.dst_stages = (BitMask)PipelineStage::DRAW_INDIRECT;
			indirect_params_barrier.src_access_mask = (BitMask)PipelineAccess::SHADER_WRITE;
			indirect_params_barrier.dst_access_mask = (BitMask)PipelineAccess::INDIRECT_COMMAND_READ;
			indirect_params_barrier.buffer_barrier_offset = 0;
			indirect_params_barrier.buffer_barrier_size = UINT64_MAX;

			PipelineResourceBarrierInfo culled_objects_barrier = {};
			culled_objects_barrier.src_stages = (BitMask)PipelineStage::COMPUTE_SHADER;
			culled_objects_barrier.dst_stages = (BitMask)PipelineStage::TASK_SHADER | (BitMask)PipelineStage::MESH_SHADER;
			culled_objects_barrier.src_access_mask = (BitMask)PipelineAccess::SHADER_WRITE;
			culled_objects_barrier.dst_access_mask = (BitMask)PipelineAccess::SHADER_READ;
			culled_objects_barrier.buffer_barrier_offset = 0;
			culled_objects_barrier.buffer_barrier_size = UINT64_MAX;

			culling_buffers_barriers.push_back(std::make_pair(indirect_params_buffer, indirect_params_barrier));
			culling_buffers_barriers.push_back(std::make_pair(culled_objects_buffer, culled_objects_barrier));
		}

		PipelineBarrierInfo culling_barrier_info = {};
		culling_barrier_info.buffer_barriers = culling_buffers_barriers;
		Renderer::InsertBarrier(culling_barrier_info);
		

		// Render 3D
		std::vector<std::pair<Shared<DeviceBuffer>, PipelineResourceBarrierInfo>> render_barriers;
		for (auto& device_render_queue : m_DeviceRenderQueue) {
			// Render survived objects
			MiscData graphics_pc = {};
			uint64* data = new uint64[3];
			data[0] = camera_data_device_address;
			data[1] = m_MeshResourcesBuffer.GetStorageBDA();
			data[2] = m_CulledDeviceRenderQueue[device_render_queue.first]->GetDeviceAddress();
			graphics_pc.data = (byte*)data;
			graphics_pc.size = sizeof uint64 * 3;

			Renderer::BeginRender({ m_CurrectMainRenderTarget, m_CurrentDepthAttachment }, m_CurrectMainRenderTarget->GetSpecification().extent, { 0, 0 }, { 0.0f, 0.0f, 0.0f, 0.0f });
			Renderer::BindSet(m_SceneDescriptorSet[Renderer::GetCurrentFrameIndex()], device_render_queue.first, 0);
			Renderer::RenderMeshTasksIndirect(device_render_queue.first, m_DeviceIndirectDrawParams[device_render_queue.first], graphics_pc);
			Renderer::EndRender(m_CurrectMainRenderTarget);

			PipelineResourceBarrierInfo culled_objects_barrier = {};
			culled_objects_barrier.src_stages = (BitMask)PipelineStage::TASK_SHADER | (BitMask)PipelineStage::MESH_SHADER;
			culled_objects_barrier.dst_stages = (BitMask)PipelineStage::COMPUTE_SHADER;
			culled_objects_barrier.src_access_mask = (BitMask)PipelineAccess::SHADER_READ;
			culled_objects_barrier.dst_access_mask = (BitMask)PipelineAccess::SHADER_WRITE;
			culled_objects_barrier.buffer_barrier_offset = 0;
			culled_objects_barrier.buffer_barrier_size = UINT64_MAX;

			PipelineResourceBarrierInfo indirect_params_barrier = {};
			indirect_params_barrier.src_stages = (BitMask)PipelineStage::DRAW_INDIRECT;
			indirect_params_barrier.dst_stages = (BitMask)PipelineStage::COMPUTE_SHADER;
			indirect_params_barrier.src_access_mask = (BitMask)PipelineAccess::INDIRECT_COMMAND_READ;
			indirect_params_barrier.dst_access_mask = (BitMask)PipelineAccess::SHADER_WRITE;
			indirect_params_barrier.buffer_barrier_offset = 0;
			indirect_params_barrier.buffer_barrier_size = UINT64_MAX;

			render_barriers.push_back(std::make_pair(m_DeviceIndirectDrawParams[device_render_queue.first], indirect_params_barrier));
			render_barriers.push_back(std::make_pair(m_CulledDeviceRenderQueue[device_render_queue.first], culled_objects_barrier));
		}

		PipelineBarrierInfo render_barrier_info = {};
		render_barrier_info.buffer_barriers = render_barriers;
		Renderer::InsertBarrier(render_barrier_info);

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
	}

	Shared<Image> SceneRenderer::GetFinalImage()
	{
		return m_RendererOutputs[Renderer::GetCurrentFrameIndex()];
	}

	uint32 SceneRenderer::AcquireResourceIndex(Shared<Image> image, SamplerFilteringMode filtering_mode)
	{
		uint32 index = m_TextureIndexAllocator->Allocate(4, 1) / sizeof uint32;

		Shared<ImageSampler> sampler;

		switch (filtering_mode)
		{
		case SamplerFilteringMode::NEAREST:		sampler = s_SamplerNearest; break;
		case SamplerFilteringMode::LINEAR:		sampler = s_SamplerLinear;  break;
		default:								OMNIFORCE_ASSERT_TAGGED(false, "Invalid sampler filtering mode"); break;
		}

		for (auto& set : m_SceneDescriptorSet)
			set->Write(0, index, image, sampler);
		m_TextureIndices.emplace(image->Handle, index);

		return index;
	}

	uint32 SceneRenderer::AcquireResourceIndex(Shared<Material> material)
	{
		return m_MaterialDataPool.Allocate(material->Handle);
	}

	uint32 SceneRenderer::AcquireResourceIndex(Shared<Mesh> mesh)
	{
		DeviceMeshData mesh_data = {};
		mesh_data.bounding_sphere =				mesh->GetBoundingSphere();
		mesh_data.meshlet_count =				mesh->GetMeshletCount();
		mesh_data.geometry_bda =				mesh->GetBuffer(MeshBufferKey::GEOMETRY)->GetDeviceAddress();
		mesh_data.attributes_bda =				mesh->GetBuffer(MeshBufferKey::ATTRIBUTES)->GetDeviceAddress();
		mesh_data.meshlets_bda =				mesh->GetBuffer(MeshBufferKey::MESHLETS)->GetDeviceAddress();
		mesh_data.micro_indices_bda =			mesh->GetBuffer(MeshBufferKey::MICRO_INDICES)->GetDeviceAddress();
		mesh_data.meshlets_cull_data_bda =		mesh->GetBuffer(MeshBufferKey::MESHLETS_CULL_DATA)->GetDeviceAddress();
		
		return m_MeshResourcesBuffer.Allocate(mesh->Handle, mesh_data);;
	}

	bool SceneRenderer::ReleaseResourceIndex(Shared<Image> image)
	{
		uint16 index = m_TextureIndices.at(image->Handle);
		m_TextureIndexAllocator->Free(sizeof(uint32) * index);
		m_TextureIndices.erase(image->Handle);

		return true;
	}

	void SceneRenderer::RenderSprite(const Sprite& sprite)
	{
		m_SpriteQueue.push_back(sprite);
	}

	void SceneRenderer::RenderObject(Shared<Pipeline> pipeline, const DeviceRenderableObject& render_data)
	{
		m_HostRenderQueue[pipeline].push_back(render_data);
	}

}