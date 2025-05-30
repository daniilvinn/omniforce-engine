#include <Scene/PathTracingSceneRenderer.h>

#include <Renderer/Renderer.h>
#include <Renderer/AccelerationStructure.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/RTPipeline.h>
#include <Renderer/DescriptorSet.h>
#include <Asset/AssetManager.h>
#include <Asset/PrimitiveMeshGenerator.h>

namespace Omni {

	Ref<ISceneRenderer> PathTracingSceneRenderer::Create(IAllocator* allocator, SceneRendererSpecification& spec)
	{
		return CreateRef<PathTracingSceneRenderer>(allocator, spec);
	}

	PathTracingSceneRenderer::PathTracingSceneRenderer(const SceneRendererSpecification& spec)
		: ISceneRenderer(spec)
	{
		PrimitiveMeshGenerator sphere_generator;
		auto [sphere_vertices, sphere_indices] = sphere_generator.GenerateIcosphere(4);

		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.size = sphere_vertices.size() * sizeof(glm::vec3);
		buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_INPUT;

		Ref<DeviceBuffer> sphere_vbo = DeviceBuffer::Create(&g_TransientAllocator, buffer_spec, sphere_vertices.data(), buffer_spec.size);

		buffer_spec.size = sphere_indices.size() * sizeof(uint32);
		Ref<DeviceBuffer> sphere_ibo = DeviceBuffer::Create(&g_TransientAllocator, buffer_spec, sphere_indices.data(), buffer_spec.size);

		BLASBuildInfo blas_build_info = {};
		blas_build_info.geometry = sphere_vbo;
		blas_build_info.indices = sphere_ibo;
		blas_build_info.index_count = sphere_indices.size();
		blas_build_info.vertex_count = sphere_vertices.size();
		blas_build_info.vertex_stride = sizeof(glm::vec3);

		m_SphereBLAS = RTAccelerationStructure::Create(&g_PersistentAllocator, blas_build_info);

		ShaderLibrary* shader_library = ShaderLibrary::Get();
		shader_library->LoadShader2(
			"PathConstruction",
			{
				"PathTracing.PathConstruction"
			},
			{}
		);
		shader_library->LoadShader2(
			"ClosestHit",
			{
				"PathTracing.PathVertex"
			},
			{}
		);
		shader_library->LoadShader2(
			"PointLightHit",
			{
				"PathTracing.PointLightHit"
			},
			{}
		);
		shader_library->LoadShader2(
			"Miss",
			{
				"PathTracing.Miss"
			},
			{}
		);

		RTPipelineSpecification rt_pipeline_spec = {};
		rt_pipeline_spec.groups = Array<RTShaderGroup>(&g_TransientAllocator);
		rt_pipeline_spec.recursion_depth = 1;

		RTShaderGroup raygen_group = {};
		raygen_group.ray_generation = shader_library->GetShader("PathConstruction");

		RTShaderGroup closest_hit_group = {};
		closest_hit_group.closest_hit = shader_library->GetShader("ClosestHit");

		RTShaderGroup point_light_hit_group = {};
		point_light_hit_group.closest_hit = shader_library->GetShader("PointLightHit");

		RTShaderGroup miss_group = {};
		miss_group.miss = shader_library->GetShader("Miss");

		rt_pipeline_spec.groups.Add(raygen_group);
		rt_pipeline_spec.groups.Add(closest_hit_group);
		rt_pipeline_spec.groups.Add(point_light_hit_group);
		rt_pipeline_spec.groups.Add(miss_group);

		m_RTPipeline = RTPipeline::Create(&g_PersistentAllocator, rt_pipeline_spec);

		// Init visibility buffer
		{
			ImageSpecification image_spec = ImageSpecification::Default();
			image_spec.usage = ImageUsage::STORAGE_IMAGE;
			image_spec.format = ImageFormat::R64_UINT;
			image_spec.extent = Renderer::GetSwapchainImage()->GetSpecification().extent;
			OMNI_DEBUG_ONLY_CODE(image_spec.debug_name = "Vis-buffer attachment");

			m_VisibilityBuffer = Image::Create(&g_PersistentAllocator, image_spec);

			for (auto& set : m_SceneDescriptorSet) {
				set->Write(1, 0, m_VisibilityBuffer, nullptr);
			}
		}

	}

	PathTracingSceneRenderer::~PathTracingSceneRenderer()
	{

	}

	void PathTracingSceneRenderer::BeginScene(Ref<Camera> camera)
	{
		m_CurrentMainRenderTarget = m_RendererOutputs[Renderer::GetCurrentFrameIndex()];
		m_CurrentDepthAttachment = m_DepthAttachments[Renderer::GetCurrentFrameIndex()];

		// Write camera data to device buffer
		if (camera) {
			m_Camera = camera;

			WeakPtr<Camera3D> camera_3D = m_Camera;

			ViewData camera_data;
			camera_data.view = m_Camera->GetViewMatrix();
			camera_data.proj = m_Camera->GetProjectionMatrix();
			camera_data.view_proj = m_Camera->GetViewProjectionMatrix();
			camera_data.position = m_Camera->GetPosition();
			camera_data.frustum = m_Camera->GenerateFrustum();
			camera_data.forward_vector = m_Camera->GetForwardVector();
			// If camera is 3D (very likely to be truth though) cast it to 3D camera and get FOV, otherwise use fixed 90 degree FOV
			camera_data.fov = m_Camera->GetType() == CameraProjectionType::PROJECTION_3D ? camera_3D->GetFOV() : glm::radians(90.0f);
			camera_data.viewport_width = m_CurrentMainRenderTarget->GetSpecification().extent.r;
			camera_data.viewport_height = m_CurrentMainRenderTarget->GetSpecification().extent.g;
			camera_data.near_clip_distance = m_Camera->GetNearClip();
			camera_data.far_clip_distance = m_Camera->GetFarClip();

			m_CameraDataBuffer->UploadData(
				m_CameraDataBuffer->GetFrameOffset(),
				&camera_data,
				sizeof camera_data
			);

			if (!m_HighLevelInstanceQueue.empty()) {
				if (memcmp(&camera_data, &m_PreviousFrameView, sizeof(ViewData)) == 0) {
					m_AccumulatedFrameCount++;
				}
				else {
					m_AccumulatedFrameCount = 1;
				}

				m_PreviousFrameView = camera_data;
			}


			DebugRenderer::SetCameraBuffer(m_CameraDataBuffer);
		}

		m_HighLevelInstanceQueue.clear();
		m_HostRenderQueue.clear();
		m_HostPointLights.clear();

		Renderer::Submit([=]() {
			m_CurrentMainRenderTarget->SetLayout(
				Renderer::GetCmdBuffer(),
				ImageLayout::COLOR_ATTACHMENT,
				PipelineStage::FRAGMENT_SHADER,
				PipelineStage::RAY_TRACING_SHADER,
				(BitMask)PipelineAccess::SHADER_READ,
				(BitMask)PipelineAccess::SHADER_WRITE
			);
		});
	}

	void PathTracingSceneRenderer::EndScene()
	{
		//Renderer::ClearImage(m_CurrentMainRenderTarget, { 0.0f, 0.0f, 0.0f, 1.0f });

		if(!(m_HighLevelInstanceQueue.empty() && m_HostPointLights.empty())) {
			// Upload data to the render queue
			m_DeviceRenderQueue->UploadData(
				m_DeviceRenderQueue->GetFrameOffset(),
				m_HostRenderQueue.data(),
				m_HostRenderQueue.size() * sizeof(InstanceRenderData)
			);

			// Copy light sources data
			uint32 num_point_lights = m_HostPointLights.size();
			m_DevicePointLights->UploadData(m_DevicePointLights->GetFrameOffset(), &num_point_lights, sizeof(num_point_lights));
			m_DevicePointLights->UploadData(m_DevicePointLights->GetFrameOffset() + 4, m_HostPointLights.data(), m_HostPointLights.size() * sizeof(PointLight));

			// Gather instances
			Array<TLASInstance> tlas_instances(&g_TransientAllocator);
			uint32 i = 0;
			for (const auto& instance : m_HighLevelInstanceQueue) {
				Ref<Mesh> mesh = AssetManager::Get()->GetAsset<Mesh>(instance.mesh_handle);

				TLASInstance tlas_instance = {};
				tlas_instance.blas = mesh->GetAccelerationStructure();
				tlas_instance.custom_index = i;
				tlas_instance.mask = 0xFF;
				tlas_instance.SBT_record_offset = 0;
				tlas_instance.transform = instance.transform;

				tlas_instances.Add(tlas_instance);
				i++;
			}
			i = 0;

			// Gather point lights
			for (const auto& light : m_HostPointLights) {
				TLASInstance tlas_instance = {};
				tlas_instance.blas = m_SphereBLAS;
				tlas_instance.custom_index = i;
				tlas_instance.mask = 0xFF;
				tlas_instance.SBT_record_offset = 1;
				tlas_instance.transform.translation = light.Position;
				tlas_instance.transform.scale = glm::vec3(light.MinRadius);

				tlas_instances.Add(tlas_instance);
				i++;
			}

			// Build TLAS
			TLASBuildInfo tlas_build_info = {};
			tlas_build_info.instances = std::move(tlas_instances);

			m_SceneTLAS = RTAccelerationStructure::Create(&g_PersistentAllocator, tlas_build_info);

			m_SceneDescriptorSet[Renderer::GetCurrentFrameIndex()]->Write(2, 0, m_SceneTLAS);

			// Trace rays
			Renderer::BindSet(m_SceneDescriptorSet[Renderer::GetCurrentFrameIndex()], m_RTPipeline, 0);

			glm::uvec3 dispatch_grid = m_CurrentMainRenderTarget->GetSpecification().extent;

			PathTracingInput* path_tracing_input = new PathTracingInput;
			path_tracing_input->View = BDA<ViewData>(m_CameraDataBuffer, m_CameraDataBuffer->GetFrameOffset());
			path_tracing_input->Instances = BDA<InstanceRenderData>(m_DeviceRenderQueue, m_DeviceRenderQueue->GetFrameOffset());
			path_tracing_input->Meshes = m_MeshResourcesBuffer.GetStorageBDA();
			path_tracing_input->RandomSeed = RandomEngine::Generate<uint32>(1);
			//path_tracing_input->RandomSeed = 123123123;
			path_tracing_input->AccumulatedFrames = glm::min(m_AccumulatedFrameCount, 8192ull);
			path_tracing_input->PointLights = BDA<ScenePointLights>(m_DevicePointLights, m_DevicePointLights->GetFrameOffset());

			MiscData pc = {};
			pc.data = (byte*)path_tracing_input;
			pc.size = sizeof(PathTracingInput);

			Renderer::DispatchRayTracing(m_RTPipeline, dispatch_grid, pc);
		}

		// Transition final image barrier
		Renderer::Submit([=]() {
			m_CurrentMainRenderTarget->SetLayout(
				Renderer::GetCmdBuffer(),
				ImageLayout::SHADER_READ_ONLY,
				PipelineStage::RAY_TRACING_SHADER,
				PipelineStage::FRAGMENT_SHADER,
				(BitMask)PipelineAccess::SHADER_WRITE,
				(BitMask)PipelineAccess::SHADER_READ
			);
		});

	}

}