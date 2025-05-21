#include <Scene/PathTracingSceneRenderer.h>

#include <Renderer/Renderer.h>
#include <Renderer/AccelerationStructure.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/RTPipeline.h>
#include <Renderer/DescriptorSet.h>

namespace Omni {

	Ref<ISceneRenderer> PathTracingSceneRenderer::Create(IAllocator* allocator, SceneRendererSpecification& spec)
	{
		return CreateRef<PathTracingSceneRenderer>(allocator, spec);
	}

	PathTracingSceneRenderer::PathTracingSceneRenderer(const SceneRendererSpecification& spec)
		: ISceneRenderer(spec)
	{
		glm::vec3 triangle_vertices[] = {
			{ 0.0f, 1.0f, 0.0f },
			{ -1.0f, -1.0f, 0.0f },
			{ 1.0f, -1.0f, 0.0f }
		};

		uint32 indices[] = { 0,1,2 };

		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.size = sizeof(triangle_vertices);
		buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_INPUT;

		Ref<DeviceBuffer> vbo = DeviceBuffer::Create(&g_TransientAllocator, buffer_spec, triangle_vertices, sizeof(triangle_vertices));

		buffer_spec.size = sizeof(indices);
		Ref<DeviceBuffer> ibo = DeviceBuffer::Create(&g_TransientAllocator, buffer_spec, indices, sizeof(indices));

		BLASBuildInfo blas_build_info = {};
		blas_build_info.geometry = vbo;
		blas_build_info.indices = ibo;
		blas_build_info.index_count = 3;
		blas_build_info.vertex_count = 3;
		blas_build_info.vertex_stride = 12;

		Ptr<RTAccelerationStructure> blas = RTAccelerationStructure::Create(&g_TransientAllocator, blas_build_info);

		TLASInstance tlas_instance = {};
		tlas_instance.blas = blas;
		tlas_instance.custom_index = 0;
		tlas_instance.mask = 0xFF;
		tlas_instance.SBT_record_offset = 0;
		tlas_instance.transform = {};

		TLASBuildInfo tlas_build_info = {};
		tlas_build_info.instances = Array<TLASInstance>(&g_TransientAllocator);
		tlas_build_info.instances.Add(tlas_instance);

		Ptr<RTAccelerationStructure> tlas = RTAccelerationStructure::Create(&g_TransientAllocator, tlas_build_info);

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

		RTShaderGroup miss_group = {};
		miss_group.miss = shader_library->GetShader("Miss");

		rt_pipeline_spec.groups.Add(raygen_group);
		rt_pipeline_spec.groups.Add(closest_hit_group);
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

		Renderer::Submit([=]() {
			m_CurrentMainRenderTarget->SetLayout(
				Renderer::GetCmdBuffer(),
				ImageLayout::COLOR_ATTACHMENT,
				PipelineStage::FRAGMENT_SHADER,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				(BitMask)PipelineAccess::UNIFORM_READ,
				(BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE
			);
		});

		glm::vec3 triangle_vertices[] = {
			{ 0.0f, 1.0f, 0.0f },
			{ -1.0f, -1.0f, 0.0f },
			{ 1.0f, -1.0f, 0.0f }
		};

		uint32 indices[] = { 0,1,2 };

		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.size = sizeof(triangle_vertices);
		buffer_spec.buffer_usage = DeviceBufferUsage::SHADER_DEVICE_ADDRESS;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		buffer_spec.flags = (BitMask)DeviceBufferFlags::AS_INPUT;

		Ref<DeviceBuffer> vbo = DeviceBuffer::Create(&g_TransientAllocator, buffer_spec, triangle_vertices, sizeof(triangle_vertices));

		buffer_spec.size = sizeof(indices);
		Ref<DeviceBuffer> ibo = DeviceBuffer::Create(&g_TransientAllocator, buffer_spec, indices, sizeof(indices));

		BLASBuildInfo blas_build_info = {};
		blas_build_info.geometry = vbo;
		blas_build_info.indices = ibo;
		blas_build_info.index_count = 3;
		blas_build_info.vertex_count = 3;
		blas_build_info.vertex_stride = 12;

		m_BLAS = RTAccelerationStructure::Create(&g_PersistentAllocator, blas_build_info);

		TLASInstance tlas_instance = {};
		tlas_instance.blas = m_BLAS;
		tlas_instance.custom_index = 0;
		tlas_instance.mask = 0xFF;
		tlas_instance.SBT_record_offset = 0;
		tlas_instance.transform.scale = {1,1,1};

		glm::mat4 null_rotation = glm::rotate(glm::mat4(1.0f), 0.0f, { 1, 0, 0 });
		glm::quat q = glm::quat_cast(null_rotation);
		tlas_instance.transform.rotation = glm::packHalf(glm::vec4{ q.x, q.y, q.y, q.w });

		TLASBuildInfo tlas_build_info = {};
		tlas_build_info.instances = Array<TLASInstance>(&g_TransientAllocator);
		tlas_build_info.instances.Add(tlas_instance);

		m_SceneTLAS = RTAccelerationStructure::Create(&g_PersistentAllocator, tlas_build_info);

		m_SceneDescriptorSet[Renderer::GetCurrentFrameIndex()]->Write(2, 0, m_SceneTLAS);
	}

	void PathTracingSceneRenderer::EndScene()
	{
		Renderer::BindSet(m_SceneDescriptorSet[Renderer::GetCurrentFrameIndex()], m_RTPipeline, 0);

		glm::uvec3 dispatch_grid = {};
		dispatch_grid.x = m_VisibilityBuffer->GetSpecification().extent.x;
		dispatch_grid.y = m_VisibilityBuffer->GetSpecification().extent.y;
		dispatch_grid.z = m_VisibilityBuffer->GetSpecification().extent.z;

		Renderer::DispatchRayTracing(m_RTPipeline, dispatch_grid, {});

		Renderer::Submit([=]() {
			m_CurrentMainRenderTarget->SetLayout(
				Renderer::GetCmdBuffer(),
				ImageLayout::SHADER_READ_ONLY,
				PipelineStage::COLOR_ATTACHMENT_OUTPUT,
				PipelineStage::ALL_COMMANDS,
				(BitMask)PipelineAccess::COLOR_ATTACHMENT_WRITE,
				(BitMask)PipelineAccess::MEMORY_READ | (BitMask)PipelineAccess::MEMORY_WRITE
			);
		});
	}

}