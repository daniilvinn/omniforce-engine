#include "../SceneRenderer.h"

#include <Renderer/DescriptorSet.h>
#include <Renderer/Pipeline.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/PipelineBarrier.h>
#include <Core/Utils.h>
#include <Threading/JobSystem.h>
#include <Asset/AssetManager.h>
#include <Asset/AssetCompressor.h>
#include <Platform/Vulkan/VulkanCommon.h>
#include <DebugUtils/DebugRenderer.h>
#include "../SceneRendererPrimitives.h"

#include <taskflow/taskflow.hpp>

namespace Omni {

	Shared<SceneRenderer> SceneRenderer::Create(const SceneRendererSpecification& spec)
	{
		return std::make_shared<SceneRenderer>(spec);
	}

	SceneRenderer::SceneRenderer(const SceneRendererSpecification& spec)
		: m_Specification(spec), m_MaterialDataPool(this, 4096 * 256 /* 256Kb of data*/), m_TextureIndexAllocator(VirtualMemoryBlock::Create(4 * UINT16_MAX)),
		m_MeshResourcesBuffer(4096 * sizeof DeviceMeshData), // allow up to 4096 meshes be allocated at once
		m_StorageImageIndexAllocator(VirtualMemoryBlock::Create(4 * UINT16_MAX))
	{
		AssetManager* asset_manager = AssetManager::Get();

		// Descriptor data
		{
			std::vector<DescriptorBinding> bindings;
			// Camera Data
			bindings.push_back({ 0, DescriptorBindingType::SAMPLED_IMAGE, UINT16_MAX, (uint64)DescriptorFlags::PARTIALLY_BOUND });
			bindings.push_back({ 1, DescriptorBindingType::STORAGE_BUFFER, 1, 0 });
			bindings.push_back({ 2, DescriptorBindingType::STORAGE_IMAGE, 1, 0 });

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
			sampler_spec.address_mode = SamplerAddressMode::REPEAT;
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
			sampler_spec.address_mode = SamplerAddressMode::REPEAT;
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
			image_spec.pixels = std::vector<byte>((byte*)&image_data, (byte*)(&image_data + 1));
			image_spec.format = ImageFormat::RGBA32_UNORM;
			image_spec.type = ImageType::TYPE_2D;
			image_spec.mip_levels = 1;
			image_spec.array_layers = 1;

			m_DummyWhiteTexture = Image::Create(image_spec, 0);
			AcquireResourceIndex(m_DummyWhiteTexture, SamplerFilteringMode::LINEAR);
		}
		// Initialize pipelines
		{
			// Load shaders
			ShaderLibrary* shader_library = ShaderLibrary::Get();
			tf::Taskflow taskflow;

			std::string shader_prefix = "Resources/shaders/";

			std::map<std::string, UUID> shader_name_to_uuid_table;

			shader_name_to_uuid_table.emplace("sprite.ofs", UUID());
			shader_name_to_uuid_table.emplace("cull_indirect.ofs", UUID());
			shader_name_to_uuid_table.emplace("PBR.ofs", UUID());
			shader_name_to_uuid_table.emplace("vis_buffer.ofs", UUID());
			shader_name_to_uuid_table.emplace("vis_material_resolve.ofs", UUID());

			// helper lambda
			auto m = [](UUID id) -> auto {
				std::map<std::string, std::string> m;
				m.emplace("__OMNI_PIPELINE_LOCAL_HASH", std::to_string(Pipeline::ComputeDeviceID(id)));
				return m;
			};

			tf::Task shaders_load_task = taskflow.emplace([&](tf::Subflow& sf) {
				sf.emplace([&, shader_name = "sprite.ofs"]() { 
					shader_library->LoadShader(shader_prefix + shader_name, m(shader_name_to_uuid_table[shader_name]));
				});
				sf.emplace([&, shader_name = "cull_indirect.ofs"]() { 
					shader_library->LoadShader(shader_prefix + shader_name, m(shader_name_to_uuid_table[shader_name]));
				});
				sf.emplace([&, shader_name = "PBR.ofs"]() { 
					shader_library->LoadShader(shader_prefix + shader_name, m(shader_name_to_uuid_table[shader_name]));
				});
				sf.emplace([&, shader_name = "vis_buffer.ofs"]() { 
					shader_library->LoadShader(shader_prefix + shader_name, m(shader_name_to_uuid_table[shader_name]));
				});
				sf.emplace([&, shader_name = "vis_material_resolve.ofs"]() { 
					shader_library->LoadShader(shader_prefix + shader_name, m(shader_name_to_uuid_table[shader_name]));
				});
			});
			JobSystem::GetExecutor()->run(taskflow).wait();

			// 2D pass
			PipelineSpecification pipeline_spec = PipelineSpecification::Default();
			pipeline_spec.shader = shader_library->GetShader("sprite.ofs");
			pipeline_spec.debug_name = "sprite pass";
			pipeline_spec.output_attachments_formats = { ImageFormat::RGB32_HDR };
			pipeline_spec.culling_mode = PipelineCullingMode::NONE;

			m_SpritePass = Pipeline::Create(pipeline_spec, shader_name_to_uuid_table["sprite.ofs"]);

			// PBR full screen
			pipeline_spec.shader = shader_library->GetShader("PBR.ofs");
			pipeline_spec.debug_name = "PBR full screen";
			pipeline_spec.color_blending_enable = false;
			pipeline_spec.depth_test_enable = false;
			pipeline_spec.depth_write_enable = false;

			m_PBRFullscreenPipeline = Pipeline::Create(pipeline_spec, shader_name_to_uuid_table["PBR.ofs"]);

			// Vis buffer pass
			pipeline_spec.shader = shader_library->GetShader("vis_buffer.ofs");
			pipeline_spec.debug_name = "Vis buffer pass";
			pipeline_spec.output_attachments_formats = {};
			pipeline_spec.culling_mode = PipelineCullingMode::BACK;
			pipeline_spec.depth_test_enable = false;
			pipeline_spec.color_blending_enable = false;

			m_VisBufferPass = Pipeline::Create(pipeline_spec, shader_name_to_uuid_table["vis_buffer.ofs"]);

			// Vis material resolve pass
			pipeline_spec.shader = shader_library->GetShader("vis_material_resolve.ofs");
			pipeline_spec.debug_name = "Vis material resolve";
			pipeline_spec.depth_write_enable = true;
			pipeline_spec.depth_test_enable = true;

			m_VisMaterialResolvePass = Pipeline::Create(pipeline_spec, shader_name_to_uuid_table["vis_material_resolve.ofs"]);

			// compute frustum culling
			pipeline_spec.type = PipelineType::COMPUTE;
			pipeline_spec.shader = shader_library->GetShader("cull_indirect.ofs");
			pipeline_spec.debug_name = "frustum cull prepass";

			m_IndirectFrustumCullPipeline = Pipeline::Create(pipeline_spec, shader_name_to_uuid_table["cull_indirect.ofs"]);
		}
		// Initialize device render queue and all buffers for indirect drawing
		{
			uint8 frames_in_flight = Renderer::GetConfig().frames_in_flight;
			
			// allow for 2^16 objects for beginning.
			// TODO: allow recreating buffer with bigger size when limit is reached
			DeviceBufferSpecification spec;
			
			// Create initial render queue
			spec.size = sizeof(DeviceRenderableObject) * std::pow(2, 16) * frames_in_flight;
			spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
			spec.heap = DeviceBufferMemoryHeap::DEVICE;
			spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
			OMNI_DEBUG_ONLY_CODE(spec.debug_name = "Device render queue buffer");

			m_DeviceRenderQueue = DeviceBuffer::Create(spec);

			// Create post-cull render queue
			spec.size = sizeof(DeviceRenderableObject) * std::pow(2, 16);
			spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
			OMNI_DEBUG_ONLY_CODE(spec.debug_name = "Device culled render queue buffer");

			m_CulledDeviceRenderQueue = DeviceBuffer::Create(spec);

			// Create indirect params buffer
			spec.size = 4 + sizeof(glm::uvec3) * std::pow(2, 16);
			spec.buffer_usage = DeviceBufferUsage::INDIRECT_PARAMS;
			OMNI_DEBUG_ONLY_CODE(spec.debug_name = "Device indirect draw params buffer");

			m_DeviceIndirectDrawParams = DeviceBuffer::Create(spec);
		}
		// Init G-Buffer
		{
			ImageSpecification image_spec = ImageSpecification::Default();
			image_spec.usage = ImageUsage::RENDER_TARGET;
			image_spec.format = ImageFormat::RGBA64_SFLOAT;
			image_spec.extent = Renderer::GetSwapchainImage()->GetSpecification().extent;
			OMNI_DEBUG_ONLY_CODE(image_spec.debug_name = "G-Buffer positions attachment");

			m_GBuffer.positions = Image::Create(image_spec);

			OMNI_DEBUG_ONLY_CODE(image_spec.debug_name = "G-Buffer normals attachment");
			m_GBuffer.normals = Image::Create(image_spec);

			image_spec.format = ImageFormat::RGBA32_UNORM;

			OMNI_DEBUG_ONLY_CODE(image_spec.debug_name = "G-Buffer base color attachment");
			m_GBuffer.base_color = Image::Create(image_spec);

			OMNI_DEBUG_ONLY_CODE(image_spec.debug_name = "G-Buffer MRO attachment");
			m_GBuffer.metallic_roughness_occlusion = Image::Create(image_spec);

			// Acquire indices
			AcquireResourceIndex(m_GBuffer.positions, SamplerFilteringMode::LINEAR);
			AcquireResourceIndex(m_GBuffer.base_color, SamplerFilteringMode::LINEAR);
			AcquireResourceIndex(m_GBuffer.normals, SamplerFilteringMode::LINEAR);
			AcquireResourceIndex(m_GBuffer.metallic_roughness_occlusion, SamplerFilteringMode::LINEAR);
		}
		// Init visibility buffer
		{
			ImageSpecification image_spec = ImageSpecification::Default();
			image_spec.usage = ImageUsage::STORAGE_IMAGE;
			image_spec.format = ImageFormat::R64_UINT;
			image_spec.extent = Renderer::GetSwapchainImage()->GetSpecification().extent;
			OMNI_DEBUG_ONLY_CODE(image_spec.debug_name = "Vis-buffer attachment");

			m_VisibilityBuffer = Image::Create(image_spec);

			for (auto& set : m_SceneDescriptorSet) {
				set->Write(2, 0, m_VisibilityBuffer, nullptr);
			}
		}
		// Init visible cluster buffer
		{
			DeviceBufferSpecification buffer_spec = {};
			buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
			buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
			buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;

			// Allocate maximum 2^25 clusters, since it is maximum index in vis buffer.
			// Actually a pretty big buffer (256 MB), so need to consider reducing its size
			// + 4 bytes for size
			buffer_spec.size = 4 + glm::pow(2, 25) * sizeof(SceneVisibleCluster) ; 

			m_VisibleClusters = DeviceBuffer::Create(buffer_spec);
		}
		// Init host light source storage
		{
			m_HostPointLights.reserve(256);

			DeviceBufferSpecification buffer_spec = {};
			buffer_spec.heap = DeviceBufferMemoryHeap::HOST;
			buffer_spec.memory_usage = DeviceBufferMemoryUsage::COHERENT_WRITE;
			buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
			buffer_spec.size = sizeof(PointLight) * 256 * Renderer::GetConfig().frames_in_flight;

			m_DevicePointLights = DeviceBuffer::Create(buffer_spec);
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
		// Clear host render queue
		m_HostRenderQueue.clear();

		// Clear active pipelines
		m_ActiveMaterialPipelines.clear();

		// Clear sprite queue
		m_SpriteQueue.clear();

		// Clear light data
		m_HostPointLights.clear();

		// Change current render target
		m_CurrectMainRenderTarget = m_RendererOutputs[Renderer::GetCurrentFrameIndex()];
		m_CurrentDepthAttachment = m_DepthAttachments[Renderer::GetCurrentFrameIndex()];

		// Write camera data to device buffer
		if (camera) {
			m_Camera = camera;

			DeviceCameraData camera_data;
			camera_data.view = m_Camera->GetViewMatrix();
			camera_data.proj = m_Camera->GetProjectionMatrix();
			camera_data.view_proj = m_Camera->GetViewProjectionMatrix();
			camera_data.position = m_Camera->GetPosition();
			camera_data.frustum = m_Camera->GenerateFrustum();
			camera_data.forward_vector = m_Camera->GetForwardVector();
			// If camera is 3D (very likely to be truth though) cast it to 3D camera and get FOV, otherwise use fixed 90 degree FOV
			camera_data.fov = m_Camera->GetType() == CameraProjectionType::PROJECTION_3D ? ShareAs<Camera3D>(m_Camera)->GetFOV() : glm::radians(90.0f);
			camera_data.viewport_width = m_CurrectMainRenderTarget->GetSpecification().extent.r;
			camera_data.viewport_height = m_CurrectMainRenderTarget->GetSpecification().extent.g;
			camera_data.near_clip_distance = m_Camera->GetNearClip();
			camera_data.far_clip_distance = m_Camera->GetFarClip();

			m_CameraDataBuffer->UploadData(
				Renderer::GetCurrentFrameIndex() * (m_CameraDataBuffer->GetSpecification().size / Renderer::GetConfig().frames_in_flight),
				&camera_data,
				sizeof camera_data
			);

			DebugRenderer::SetCameraBuffer(m_CameraDataBuffer);
		}

		// Change layout of render target
		Renderer::Submit([=]() {
			m_CurrectMainRenderTarget->SetLayout(
				Renderer::GetCmdBuffer(),
				ImageLayout::COLOR_ATTACHMENT,
				PipelineStage::FRAGMENT_SHADER,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				(BitMask)PipelineAccess::UNIFORM_READ,
				(BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE
			);
		});

	}

	void SceneRenderer::EndScene()
	{
		uint64 camera_data_device_address = m_CameraDataBuffer->GetDeviceAddress() + m_CameraDataBuffer->GetFrameOffset();

		// Copy data to render queues and clear culling pipeline output buffers
		{
			std::vector<std::pair<Shared<DeviceBuffer>, PipelineResourceBarrierInfo>> buffers_clear_barriers;
			m_DeviceRenderQueue->UploadData(
				m_DeviceRenderQueue->GetFrameOffset(),
				m_HostRenderQueue.data(),
				m_HostRenderQueue.size() * sizeof DeviceRenderableObject
			);

			Renderer::Submit([=]() {
				m_CulledDeviceRenderQueue->Clear(Renderer::GetCmdBuffer(), 0, 4, 0);
				m_DeviceIndirectDrawParams->Clear(Renderer::GetCmdBuffer(), 0, 4, 0);
				m_VisibleClusters->Clear(Renderer::GetCmdBuffer(), 0, 4, 0);
			});

			Renderer::ClearImage(m_VisibilityBuffer, { 0.0f, 0.0f, 0.0f, 0.0f });

			// Wait on transfer before cull compute pass
			PipelineResourceBarrierInfo indirect_params_barrier = {};
			indirect_params_barrier.src_stages = (BitMask)PipelineStage::TRANSFER;
			indirect_params_barrier.dst_stages = (BitMask)PipelineStage::COMPUTE_SHADER;
			indirect_params_barrier.src_access_mask = (BitMask)PipelineAccess::TRANSFER_WRITE;
			indirect_params_barrier.dst_access_mask = (BitMask)PipelineAccess::SHADER_WRITE | (BitMask)PipelineAccess::SHADER_READ;
			indirect_params_barrier.buffer_barrier_offset = 0;
			indirect_params_barrier.buffer_barrier_size = m_DeviceIndirectDrawParams->GetSpecification().size;

			PipelineResourceBarrierInfo culled_objects_barrier = {};
			culled_objects_barrier.src_stages = (BitMask)PipelineStage::TRANSFER;
			culled_objects_barrier.dst_stages = (BitMask)PipelineStage::COMPUTE_SHADER;
			culled_objects_barrier.src_access_mask = (BitMask)PipelineAccess::TRANSFER_WRITE;
			culled_objects_barrier.dst_access_mask = (BitMask)PipelineAccess::SHADER_WRITE;
			culled_objects_barrier.buffer_barrier_offset = 0;
			culled_objects_barrier.buffer_barrier_size = m_CulledDeviceRenderQueue->GetSpecification().size;

			buffers_clear_barriers.push_back(std::make_pair(m_DeviceIndirectDrawParams, indirect_params_barrier));
			buffers_clear_barriers.push_back(std::make_pair(m_CulledDeviceRenderQueue, culled_objects_barrier));
			
			// Early transition of PBR attachments' layout
			PipelineResourceBarrierInfo pbr_attachment_barrier = {};
			pbr_attachment_barrier.src_stages = (BitMask)PipelineStage::FRAGMENT_SHADER;
			pbr_attachment_barrier.dst_stages = (BitMask)PipelineStage::COLOR_ATTACHMENT_OUTPUT;
			pbr_attachment_barrier.src_access_mask = (BitMask)PipelineAccess::UNIFORM_READ;
			pbr_attachment_barrier.dst_access_mask = (BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE;
			pbr_attachment_barrier.new_image_layout = ImageLayout::GENERAL;

			PipelineBarrierInfo clear_buffers_barrier_info = {};
			clear_buffers_barrier_info.buffer_barriers = buffers_clear_barriers;
			clear_buffers_barrier_info.image_barriers.push_back(std::make_pair(m_GBuffer.positions, pbr_attachment_barrier));
			clear_buffers_barrier_info.image_barriers.push_back(std::make_pair(m_GBuffer.base_color, pbr_attachment_barrier));
			clear_buffers_barrier_info.image_barriers.push_back(std::make_pair(m_GBuffer.normals, pbr_attachment_barrier));
			clear_buffers_barrier_info.image_barriers.push_back(std::make_pair(m_GBuffer.metallic_roughness_occlusion, pbr_attachment_barrier));

			Renderer::InsertBarrier(clear_buffers_barrier_info);
		}

		// Copy light sources data
		m_DevicePointLights->UploadData(m_DevicePointLights->GetFrameOffset(), m_HostPointLights.data(), m_HostPointLights.size() * sizeof PointLight);

		// Cull 3D
		{
			std::vector<std::pair<Shared<DeviceBuffer>, PipelineResourceBarrierInfo>> culling_buffers_barriers;

			// Prepare push constants
			IndirectFrustumCullPassPushContants* compute_push_constants_data = new IndirectFrustumCullPassPushContants;
			compute_push_constants_data->camera_data_bda = camera_data_device_address;
			compute_push_constants_data->mesh_data_bda = m_MeshResourcesBuffer.GetStorageBDA();
			compute_push_constants_data->render_objects_data_bda = m_DeviceRenderQueue->GetDeviceAddress() + m_DeviceRenderQueue->GetFrameOffset();
			compute_push_constants_data->original_object_count = m_HostRenderQueue.size();
			compute_push_constants_data->culled_objects_bda = m_CulledDeviceRenderQueue->GetDeviceAddress();
			compute_push_constants_data->output_buffer_bda = m_DeviceIndirectDrawParams->GetDeviceAddress();

			MiscData compute_pc = {};
			compute_pc.data = (byte*)compute_push_constants_data;
			compute_pc.size = sizeof IndirectFrustumCullPassPushContants;

			// Submit a dispatch
			uint32 num_work_groups = (m_HostRenderQueue.size() + 255) / 256;
			Renderer::DispatchCompute(m_IndirectFrustumCullPipeline, { num_work_groups, 1, 1 }, compute_pc);

			// Prepare barriers, so visibility buffer pass doesn't starts before culling is fully finished
			PipelineResourceBarrierInfo indirect_params_barrier = {};
			indirect_params_barrier.src_stages = (BitMask)PipelineStage::COMPUTE_SHADER;
			indirect_params_barrier.dst_stages = (BitMask)PipelineStage::DRAW_INDIRECT;
			indirect_params_barrier.src_access_mask = (BitMask)PipelineAccess::SHADER_WRITE;
			indirect_params_barrier.dst_access_mask = (BitMask)PipelineAccess::INDIRECT_COMMAND_READ;
			indirect_params_barrier.buffer_barrier_offset = 0;
			indirect_params_barrier.buffer_barrier_size = m_DeviceIndirectDrawParams->GetSpecification().size;

			PipelineResourceBarrierInfo culled_objects_barrier = {};
			culled_objects_barrier.src_stages = (BitMask)PipelineStage::COMPUTE_SHADER;
			culled_objects_barrier.dst_stages = (BitMask)PipelineStage::TASK_SHADER | (BitMask)PipelineStage::MESH_SHADER;
			culled_objects_barrier.src_access_mask = (BitMask)PipelineAccess::SHADER_WRITE;
			culled_objects_barrier.dst_access_mask = (BitMask)PipelineAccess::SHADER_READ;
			culled_objects_barrier.buffer_barrier_offset = 0;
			culled_objects_barrier.buffer_barrier_size = m_CulledDeviceRenderQueue->GetSpecification().size;

			culling_buffers_barriers.push_back(std::make_pair(m_DeviceIndirectDrawParams, indirect_params_barrier));
			culling_buffers_barriers.push_back(std::make_pair(m_CulledDeviceRenderQueue, culled_objects_barrier));

			PipelineBarrierInfo culling_barrier_info = {};
			culling_barrier_info.buffer_barriers = culling_buffers_barriers;
			Renderer::InsertBarrier(culling_barrier_info);
		}

		// Render 3D
		// Vis buffer pass
		{
			Renderer::BeginRender(
				{ },
				m_CurrectMainRenderTarget->GetSpecification().extent,
				{ 0, 0 },
				{ 0.2f, 0.2f, 0.3f, 1.0 }
			);

			// Render survived objects
			MiscData graphics_pc = {};
			uint64* data = new uint64[4];

			data[0] = camera_data_device_address;
			data[1] = m_MeshResourcesBuffer.GetStorageBDA();
			data[2] = m_CulledDeviceRenderQueue->GetDeviceAddress();
			data[3] = m_VisibleClusters->GetDeviceAddress();

			graphics_pc.data = (byte*)data;
			graphics_pc.size = sizeof uint64 * 4;

			Renderer::BindSet(m_SceneDescriptorSet[Renderer::GetCurrentFrameIndex()], m_VisBufferPass, 0);
			Renderer::RenderMeshTasksIndirect(m_VisBufferPass, m_DeviceIndirectDrawParams, graphics_pc);

			Renderer::EndRender({});
		}
		if (!IsInDebugMode()) {
			// Visible materials resolve pass
			{
				Renderer::BeginRender(
					{ m_CurrentDepthAttachment },
					m_CurrectMainRenderTarget->GetSpecification().extent,
					{ 0, 0 },
					{ 0.0f, 0.0f, 0.0f, 0.0f }
				);

				MiscData pc = {};
				uint64* data = new uint64[2];
				data[0] = m_CulledDeviceRenderQueue->GetDeviceAddress();
				data[1] = m_VisibleClusters->GetDeviceAddress();

				pc.data = (byte*)data;
				pc.size = sizeof(uint64) * 2;

				Renderer::BindSet(m_SceneDescriptorSet[Renderer::GetCurrentFrameIndex()], m_VisMaterialResolvePass, 0);
				Renderer::RenderQuads(m_VisMaterialResolvePass, pc);

				Renderer::EndRender({ m_CurrentDepthAttachment });

			}
			// PBR full screen pass
			{
				// Transition PBR attachments to SHADER_READ_ONLY layout, so we can sample them during lighting pass
				PipelineResourceBarrierInfo pbr_attachment_barrier = {};
				pbr_attachment_barrier.new_image_layout = ImageLayout::SHADER_READ_ONLY;
				pbr_attachment_barrier.src_stages = (BitMask)PipelineStage::COLOR_ATTACHMENT_OUTPUT;
				pbr_attachment_barrier.dst_stages = (BitMask)PipelineStage::FRAGMENT_SHADER;
				pbr_attachment_barrier.src_access_mask = (BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE;
				pbr_attachment_barrier.dst_access_mask = (BitMask)PipelineAccess::UNIFORM_READ;

				PipelineBarrierInfo render_barrier_info = {};
				render_barrier_info.image_barriers.push_back(std::make_pair(m_GBuffer.positions, pbr_attachment_barrier));
				render_barrier_info.image_barriers.push_back(std::make_pair(m_GBuffer.base_color, pbr_attachment_barrier));
				render_barrier_info.image_barriers.push_back(std::make_pair(m_GBuffer.normals, pbr_attachment_barrier));
				render_barrier_info.image_barriers.push_back(std::make_pair(m_GBuffer.metallic_roughness_occlusion, pbr_attachment_barrier));
				Renderer::InsertBarrier(render_barrier_info);

				// Prepare push constants
				PBRFullScreenPassPushConstants* pbr_push_constants = new PBRFullScreenPassPushConstants;
				pbr_push_constants->camera_data_bda = camera_data_device_address;
				pbr_push_constants->point_lights_bda = m_DevicePointLights->GetDeviceAddress() + m_DevicePointLights->GetFrameOffset();
				pbr_push_constants->positions_texture_index = GetTextureIndex(m_GBuffer.positions->Handle);
				pbr_push_constants->base_color_texture_index = GetTextureIndex(m_GBuffer.base_color->Handle);
				pbr_push_constants->normal_texture_index = GetTextureIndex(m_GBuffer.normals->Handle);
				pbr_push_constants->metallic_roughness_occlusion_texture_index = GetTextureIndex(m_GBuffer.metallic_roughness_occlusion->Handle);
				pbr_push_constants->point_light_count = m_HostPointLights.size();

				MiscData pbr_pc = {};
				pbr_pc.data = (byte*)pbr_push_constants;
				pbr_pc.size = sizeof PBRFullScreenPassPushConstants;

				// Submit full screen quad
				Renderer::BeginRender({ m_CurrectMainRenderTarget }, m_CurrectMainRenderTarget->GetSpecification().extent, { 0, 0 }, { INFINITY, INFINITY, INFINITY, 1.0f });
				Renderer::BindSet(m_SceneDescriptorSet[Renderer::GetCurrentFrameIndex()], m_PBRFullscreenPipeline, 0);
				Renderer::RenderQuads(m_PBRFullscreenPipeline, pbr_pc);
				Renderer::EndRender(m_CurrectMainRenderTarget);
			}
			// Render 2D
			{
				Renderer::BeginRender({ m_CurrectMainRenderTarget, m_CurrentDepthAttachment }, m_CurrectMainRenderTarget->GetSpecification().extent, { 0, 0 }, { 1.0f, 1.0f, 1.0f, 0.0f });
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
		}
		else {
			for (auto& device_render_queue : m_DeviceRenderQueue)
				DebugRenderer::RenderSceneDebugView(m_CameraDataBuffer, m_MeshResourcesBuffer.GetStorage(), m_CulledDeviceRenderQueue[device_render_queue.first], m_DeviceIndirectDrawParams[device_render_queue.first], m_CurrentDebugMode);
		}

		DebugRenderer::Render(m_CurrectMainRenderTarget, m_CurrentDepthAttachment, IsInDebugMode() ? fvec4{0.0f, 0.0f, 0.0f, 1.0f} : fvec4{0.0f, 0.0f, 0.0f, 0.0f});

		Renderer::Submit([=]() {
			m_CurrectMainRenderTarget->SetLayout(
				Renderer::GetCmdBuffer(),
				ImageLayout::SHADER_READ_ONLY,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				PipelineStage::ALL_COMMANDS,
				(BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE,
				(BitMask)PipelineAccess::MEMORY_READ | (BitMask)PipelineAccess::MEMORY_WRITE
			);
		});
	}

	Shared<Image> SceneRenderer::GetFinalImage()
	{
		return m_RendererOutputs[Renderer::GetCurrentFrameIndex()];
	}

	uint32 SceneRenderer::AcquireResourceIndex(Shared<Image> image, SamplerFilteringMode filtering_mode)
	{
		std::lock_guard lock(m_Mutex);
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
		std::lock_guard lock(m_Mutex);

		DeviceMeshData mesh_data = {};
		mesh_data.lod_distance_multiplier = 1.0f;
		mesh_data.bounding_sphere = mesh->GetBoundingSphere();
		mesh_data.meshlet_count = mesh->GetMeshletCount();
		mesh_data.quantization_grid_size = mesh->GetQuantizationGridSize();
		mesh_data.geometry_bda = mesh->GetBuffer(MeshBufferKey::GEOMETRY)->GetDeviceAddress();
		mesh_data.attributes_bda = mesh->GetBuffer(MeshBufferKey::ATTRIBUTES)->GetDeviceAddress();
		mesh_data.meshlets_bda = mesh->GetBuffer(MeshBufferKey::MESHLETS)->GetDeviceAddress();
		mesh_data.micro_indices_bda = mesh->GetBuffer(MeshBufferKey::MICRO_INDICES)->GetDeviceAddress();
		mesh_data.meshlets_cull_data_bda = mesh->GetBuffer(MeshBufferKey::MESHLETS_CULL_DATA)->GetDeviceAddress();

		return m_MeshResourcesBuffer.Allocate(mesh->Handle, mesh_data);
	}

	uint32 SceneRenderer::AcquireResourceIndex(Shared<Image> image)
	{
		OMNIFORCE_ASSERT_TAGGED(false, "Not implemented");
		std::unreachable();
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
		m_ActiveMaterialPipelines.emplace(pipeline);
		m_HostRenderQueue.emplace_back(render_data);
	}

}