#include <Foundation/Common.h>
#include <DebugUtils/DebugRenderer.h>

#include <Core/Utils.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/Renderer.h>
#include <Asset/PrimitiveMeshGenerator.h>
#include <Asset/MeshPreprocessor.h>
#include <Shaders/Shared/Transform.glslh>

#include <imgui.h>
#include <glm/gtc/packing.hpp>

namespace Omni {

	void DebugRenderer::Init()
	{
		renderer = new DebugRenderer;
	}

	void DebugRenderer::Shutdown()
	{
		delete renderer;
	}

	DebugRenderer::DebugRenderer()
	{
		// Init pipeline
		ShaderLibrary* shader_library = ShaderLibrary::Get();
		shader_library->LoadShader("Resources/Shaders/Source/Wireframe.ofs", { {"__OMNI_PIPELINE_LOCAL_HASH", std::to_string(Pipeline::ComputeDeviceID(UUID()))} });
		shader_library->LoadShader("Resources/Shaders/Source/ClusterDebugView.ofs", { {"__OMNI_PIPELINE_LOCAL_HASH", std::to_string(Pipeline::ComputeDeviceID(UUID()))} });

		DeviceBufferLayoutElement element("position", DeviceDataType::FLOAT3);
		DeviceBufferLayout buffer_layout(std::vector{ element });

		PipelineSpecification pipeline_spec = PipelineSpecification::Default();
		pipeline_spec.culling_mode = PipelineCullingMode::NONE;
		pipeline_spec.debug_name = "debug renderer wireframe";
		pipeline_spec.line_width = 3.5f;
		pipeline_spec.output_attachments_formats = { ImageFormat::RGB32_HDR };
		pipeline_spec.topology = PipelineTopology::LINES;
		pipeline_spec.shader = shader_library->GetShader("Wireframe.ofs");
		pipeline_spec.input_layout = buffer_layout;
		pipeline_spec.depth_test_enable = true;
		pipeline_spec.color_blending_enable = false;

		m_WireframePipeline = Pipeline::Create(&g_PersistentAllocator, pipeline_spec);

		pipeline_spec.culling_mode = PipelineCullingMode::BACK;
		pipeline_spec.debug_name = "Cluster debug view";
		pipeline_spec.topology = PipelineTopology::TRIANGLES;
		pipeline_spec.shader = shader_library->GetShader("ClusterDebugView.ofs");
		pipeline_spec.input_layout = {};

		m_DebugViewPipeline = Pipeline::Create(&g_PersistentAllocator, pipeline_spec);

		PrimitiveMeshGenerator mesh_generator;
		MeshPreprocessor mesh_preprocessor;

		// Icosphere
		auto icosphere_data = mesh_generator.GenerateIcosphere(2);

		std::vector<byte> vertex_data(icosphere_data.first.size() * sizeof(glm::vec3));
		memcpy(vertex_data.data(), icosphere_data.first.data(), vertex_data.size());

		// Get rid of index buffer
		std::vector<byte> primitive_vertices(icosphere_data.second.size() * sizeof(glm::vec3));
		mesh_preprocessor.RemapVertices(&primitive_vertices, &vertex_data, 12, &icosphere_data.second);
		
		// Convert to line topology
		vertex_data.resize(icosphere_data.second.size() * sizeof(glm::vec3) * 2);
		mesh_preprocessor.ConvertToLineTopology(&vertex_data, &primitive_vertices, sizeof(glm::vec3));

		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.buffer_usage = DeviceBufferUsage::VERTEX_BUFFER;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.size = vertex_data.size();

		m_IcosphereMesh = DeviceBuffer::Create(&g_PersistentAllocator, buffer_spec, vertex_data.data(), buffer_spec.size);

		// Cube
		auto cube_data = mesh_generator.GenerateCube();

		vertex_data.resize(cube_data.first.size() * sizeof(glm::vec3));

		memcpy(vertex_data.data(), cube_data.first.data(), vertex_data.size());

		primitive_vertices.resize(cube_data.second.size() * sizeof(glm::vec3));
		mesh_preprocessor.RemapVertices(&primitive_vertices, &vertex_data, 12, &cube_data.second);

		vertex_data.resize(cube_data.second.size() * sizeof(glm::vec3) * 2);
		mesh_preprocessor.ConvertToLineTopology(&vertex_data, &primitive_vertices, sizeof(glm::vec3));

		buffer_spec.size = vertex_data.size();

		m_CubeMesh = DeviceBuffer::Create(&g_PersistentAllocator, buffer_spec, vertex_data.data(), buffer_spec.size);
	}

	DebugRenderer::~DebugRenderer()
	{
		m_IcosphereMesh->Destroy();
		m_CubeMesh->Destroy();
	}

	void DebugRenderer::SetCameraBuffer(Ref<DeviceBuffer> buffer)
	{
		renderer->m_CameraBuffer = buffer;
	}

	void DebugRenderer::RenderWireframeSphere(const glm::vec3& position, float radius, const glm::vec3& color)
	{
		renderer->m_DebugRequests.push_back([=]() {
			GLSL::Transform trs = {};
			trs.translation = position;
			trs.rotation = glm::packHalf(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			trs.scale = { radius, radius, radius };

			glm::u8vec3 encoded_color = { color.r * 255, color.g * 255, color.b * 255 };

			WireframePushConstants* pc_data = new WireframePushConstants;
			memset(pc_data, 0u, sizeof(WireframePushConstants));
			pc_data->camera_data_bda = renderer->m_CameraBuffer->GetDeviceAddress() + renderer->m_CameraBuffer->GetFrameOffset();
			pc_data->trs = trs;
			pc_data->lines_color = encoded_color;
			
			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof(WireframePushConstants);

			Renderer::RenderUnindexed(renderer->m_WireframePipeline, renderer->m_IcosphereMesh, pcs);
		});
	}

	void DebugRenderer::RenderWireframeBox(const glm::vec3& translation, const glm::quat rotation, const glm::vec3 scale, const glm::vec3& color)
	{
		renderer->m_DebugRequests.push_back([=]() {
			GLSL::Transform trs = {};
			trs.translation = translation;
			trs.rotation = glm::packHalf(glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w));
			trs.scale = scale;

			glm::u8vec3 encoded_color = { color.r * 255, color.g * 255, color.b * 255 };

			WireframePushConstants* pc_data = new WireframePushConstants;
			memset(pc_data, 0u, sizeof(WireframePushConstants));
			pc_data->camera_data_bda = renderer->m_CameraBuffer->GetDeviceAddress() + renderer->m_CameraBuffer->GetFrameOffset();
			pc_data->trs = trs;
			pc_data->lines_color = encoded_color;

			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof(WireframePushConstants);

			Renderer::RenderUnindexed(renderer->m_WireframePipeline, renderer->m_CubeMesh, pcs);
		});
	};

	void DebugRenderer::RenderWireframeLines(Ref<DeviceBuffer> vbo, const glm::vec3& translation, const glm::quat rotation, const glm::vec3 scale, const glm::vec3& color)
	{
		renderer->m_DebugRequests.push_back([=]() {
			GLSL::Transform trs = {};
			trs.translation = translation;
			trs.rotation = glm::packHalf(glm::vec4( rotation.x, rotation.y, rotation.z, rotation.w ) );
			trs.scale = scale;

			glm::u8vec3 encoded_color = { color.r * 255, color.g * 255, color.b * 255 };

			WireframePushConstants* pc_data = new WireframePushConstants;
			memset(pc_data, 0u, sizeof(WireframePushConstants));
			pc_data->camera_data_bda = renderer->m_CameraBuffer->GetDeviceAddress() + renderer->m_CameraBuffer->GetFrameOffset();
			pc_data->trs = trs;
			pc_data->lines_color = encoded_color;

			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof(WireframePushConstants);

			Renderer::RenderUnindexed(renderer->m_WireframePipeline, vbo, pcs);
		});
	}

	void DebugRenderer::RenderSceneDebugView(Ref<DeviceBuffer> visible_clusters, DebugSceneView mode, Ref<DescriptorSet> descriptor_set)
	{
		renderer->m_DebugRequests.push_back([=]() {

			uint64* pc_data = new uint64[2];

			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof(uint64) * 2;

			pc_data[0] = uint64(mode);
			pc_data[1] = uint64(visible_clusters->GetDeviceAddress());

			Renderer::BindSet(descriptor_set, renderer->m_DebugViewPipeline, 0);
			Renderer::RenderQuads(renderer->m_DebugViewPipeline, pcs);
		});
	}

	void DebugRenderer::Render(Ref<Image> target, Ref<Image> depth_target, fvec4 clear_value)
	{
		Renderer::BeginRender({ target, depth_target }, target->GetSpecification().extent, { 0,0 }, clear_value);

		for (auto& request : renderer->m_DebugRequests)
			request();

		Renderer::EndRender(target);

		renderer->m_DebugRequests.clear();
	}

}