#include "../DebugRenderer.h"

#include <Core/Utils.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/Renderer.h>
#include <Asset/PrimitiveMeshGenerator.h>
#include <Asset/MeshPreprocessor.h>

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
		shader_library->LoadShader("Resources/shaders/wireframe.ofs");
		shader_library->LoadShader("Resources/shaders/cluster_debug_view.ofs");

		DeviceBufferLayoutElement element("position", DeviceDataType::FLOAT3);
		DeviceBufferLayout buffer_layout(std::vector{ element });

		PipelineSpecification pipeline_spec = PipelineSpecification::Default();
		pipeline_spec.culling_mode = PipelineCullingMode::NONE;
		pipeline_spec.debug_name = "debug renderer wireframe";
		pipeline_spec.line_width = 3.5f;
		pipeline_spec.output_attachments_formats = { ImageFormat::RGB32_HDR };
		pipeline_spec.topology = PipelineTopology::LINES;
		pipeline_spec.shader = shader_library->GetShader("wireframe.ofs");
		pipeline_spec.input_layout = buffer_layout;
		pipeline_spec.depth_test_enable = true;
		pipeline_spec.color_blending_enable = false;

		m_WireframePipeline = Pipeline::Create(pipeline_spec);

		pipeline_spec.culling_mode = PipelineCullingMode::BACK;
		pipeline_spec.debug_name = "cluster debug view";
		pipeline_spec.topology = PipelineTopology::TRIANGLES;
		pipeline_spec.shader = shader_library->GetShader("cluster_debug_view.ofs");

		m_DebugViewPipeline = Pipeline::Create(pipeline_spec);

		PrimitiveMeshGenerator mesh_generator;
		MeshPreprocessor mesh_preprocessor;

		// Icosphere
		auto icosphere_data = mesh_generator.GenerateIcosphere(2);

		std::vector<byte> vertex_data(icosphere_data.first.size() * sizeof(glm::vec3));
		memcpy(vertex_data.data(), icosphere_data.first.data(), vertex_data.size());

		// Get rid of index buffer
		std::vector<byte> primitive_vertices(icosphere_data.second.size() * sizeof glm::vec3);
		mesh_preprocessor.RemapVertices(&primitive_vertices, &vertex_data, 12, &icosphere_data.second);
		
		// Convert to line topology
		vertex_data.resize(icosphere_data.second.size() * sizeof(glm::vec3) * 2);
		mesh_preprocessor.ConvertToLineTopology(&vertex_data, &primitive_vertices, sizeof glm::vec3);

		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.buffer_usage = DeviceBufferUsage::VERTEX_BUFFER;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.size = vertex_data.size();

		m_IcosphereMesh = DeviceBuffer::Create(buffer_spec, vertex_data.data(), buffer_spec.size);

		// Cube
		auto cube_data = mesh_generator.GenerateCube();

		vertex_data.resize(cube_data.first.size() * sizeof glm::vec3);

		memcpy(vertex_data.data(), cube_data.first.data(), vertex_data.size());

		primitive_vertices.resize(cube_data.second.size() * sizeof glm::vec3);
		mesh_preprocessor.RemapVertices(&primitive_vertices, &vertex_data, 12, &cube_data.second);

		vertex_data.resize(cube_data.second.size() * sizeof(glm::vec3) * 2);
		mesh_preprocessor.ConvertToLineTopology(&vertex_data, &primitive_vertices, sizeof glm::vec3);

		buffer_spec.size = vertex_data.size();

		m_CubeMesh = DeviceBuffer::Create(buffer_spec, vertex_data.data(), buffer_spec.size);
	}

	DebugRenderer::~DebugRenderer()
	{

	}

	void DebugRenderer::SetCameraBuffer(Shared<DeviceBuffer> buffer)
	{
		renderer->m_CameraBuffer = buffer;
	}

	void DebugRenderer::RenderWireframeSphere(const glm::vec3& position, float radius, const glm::vec3& color)
	{
		renderer->m_DebugRequests.push_back([=]() {
			glm::quat identity_quat = {};

			TRS trs = {};
			trs.translation = position;
			trs.rotation = { glm::packSnorm2x16({ identity_quat.x, identity_quat.y }), glm::packSnorm2x16({ identity_quat.z, identity_quat.w }) };
			trs.scale = { radius, radius, radius };

			glm::u8vec3 encoded_color = { color.r * 255, color.g * 255, color.b * 255 };

			WireframePushConstants* pc_data = new WireframePushConstants;
			memset(pc_data, 0u, sizeof WireframePushConstants);
			pc_data->camera_data_bda = renderer->m_CameraBuffer->GetDeviceAddress() + renderer->m_CameraBuffer->GetFrameOffset();
			pc_data->trs = trs;
			pc_data->lines_color = encoded_color;
			
			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof WireframePushConstants;

			Renderer::RenderUnindexed(renderer->m_WireframePipeline, renderer->m_IcosphereMesh, pcs);
		});
	}

	void DebugRenderer::RenderWireframeBox(const glm::vec3& translation, const glm::quat rotation, const glm::vec3 scale, const glm::vec3& color)
	{
		renderer->m_DebugRequests.push_back([=]() {
			TRS trs = {};
			trs.translation = translation;
			trs.rotation = { glm::packSnorm2x16({ rotation.x, rotation.y }), glm::packSnorm2x16({ rotation.z, rotation.w }) };
			trs.scale = scale;

			glm::u8vec3 encoded_color = { color.r * 255, color.g * 255, color.b * 255 };

			WireframePushConstants* pc_data = new WireframePushConstants;
			memset(pc_data, 0u, sizeof WireframePushConstants);
			pc_data->camera_data_bda = renderer->m_CameraBuffer->GetDeviceAddress() + renderer->m_CameraBuffer->GetFrameOffset();
			pc_data->trs = trs;
			pc_data->lines_color = encoded_color;

			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof WireframePushConstants;

			Renderer::RenderUnindexed(renderer->m_WireframePipeline, renderer->m_CubeMesh, pcs);
		});
	};

	void DebugRenderer::RenderWireframeLines(Shared<DeviceBuffer> vbo, const glm::vec3& translation, const glm::quat rotation, const glm::vec3 scale, const glm::vec3& color)
	{
		renderer->m_DebugRequests.push_back([=]() {
			TRS trs = {};
			trs.translation = translation;
			trs.rotation = { glm::packSnorm2x16({ rotation.x, rotation.y }), glm::packSnorm2x16({ rotation.z, rotation.w }) };
			trs.scale = scale;

			glm::u8vec3 encoded_color = { color.r * 255, color.g * 255, color.b * 255 };

			WireframePushConstants* pc_data = new WireframePushConstants;
			memset(pc_data, 0u, sizeof WireframePushConstants);
			pc_data->camera_data_bda = renderer->m_CameraBuffer->GetDeviceAddress() + renderer->m_CameraBuffer->GetFrameOffset();
			pc_data->trs = trs;
			pc_data->lines_color = encoded_color;

			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof WireframePushConstants;

			Renderer::RenderUnindexed(renderer->m_WireframePipeline, vbo, pcs);
		});
	}

	void DebugRenderer::RenderSceneDebugView(Shared<DeviceBuffer> camera_data, Shared<DeviceBuffer> mesh_data, Shared<DeviceBuffer> render_queue, Shared<DeviceBuffer> indirect_params, DebugSceneView mode)
	{
		renderer->m_DebugRequests.push_back([=]() {

			uint64* pc_data = new uint64[4];
			memset(pc_data, 0u, sizeof uint64 * 4);
			
			pc_data[0] = camera_data->GetDeviceAddress() + camera_data->GetFrameOffset();
			pc_data[1] = mesh_data->GetDeviceAddress();
			pc_data[2] = render_queue->GetDeviceAddress();
			pc_data[3] = uint64(mode);

			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof uint64 * 4;

			Renderer::RenderMeshTasksIndirect(renderer->m_DebugViewPipeline, indirect_params, pcs);
		});
	}

	void DebugRenderer::Render(Shared<Image> target, Shared<Image> depth_target, fvec4 clear_value)
	{
		Renderer::BeginRender({ target, depth_target }, target->GetSpecification().extent, { 0,0 }, clear_value);

		for (auto& request : renderer->m_DebugRequests)
			request();

		Renderer::EndRender(target);

		renderer->m_DebugRequests.clear();
	}

}