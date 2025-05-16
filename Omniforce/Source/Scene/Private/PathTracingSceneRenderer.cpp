#include <Scene/PathTracingSceneRenderer.h>

#include <Renderer/AccelerationStructure.h>
#include <Renderer/DeviceBuffer.h>

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

		Ptr<AccelerationStructure> blas = AccelerationStructure::Create(&g_TransientAllocator, blas_build_info);

		TLASInstance tlas_instance = {};
		tlas_instance.blas = blas;
		tlas_instance.custom_index = 0;
		tlas_instance.mask = 0xFF;
		tlas_instance.SBT_record_offset = 0;
		tlas_instance.transform = {};

		TLASBuildInfo tlas_build_info = {};
		tlas_build_info.instances = Array<TLASInstance>(&g_TransientAllocator);
		tlas_build_info.instances.Add(tlas_instance);

		Ptr<AccelerationStructure> tlas = AccelerationStructure::Create(&g_TransientAllocator, tlas_build_info);
	}

	PathTracingSceneRenderer::~PathTracingSceneRenderer()
	{

	}

	void PathTracingSceneRenderer::BeginScene(Ref<Camera> camera)
	{

	}

	void PathTracingSceneRenderer::EndScene()
	{

	}

	void PathTracingSceneRenderer::RenderObject(Ref<Pipeline> pipeline, const GLSL::RenderObjectData& render_data)
	{

	}

}