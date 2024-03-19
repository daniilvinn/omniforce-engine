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

		DeviceBufferLayoutElement element("position", DeviceDataType::FLOAT3);
		DeviceBufferLayout buffer_layout(std::vector{ element });

		PipelineSpecification pipeline_spec = PipelineSpecification::Default();
		pipeline_spec.culling_mode = PipelineCullingMode::NONE;
		pipeline_spec.debug_name = "debug renderer wireframe";
		pipeline_spec.line_width = 2.0f;
		pipeline_spec.output_attachments_formats = { ImageFormat::RGB32_HDR };
		pipeline_spec.topology = PipelineTopology::LINES;
		pipeline_spec.shader = shader_library->GetShader("wireframe.ofs");
		pipeline_spec.input_layout = buffer_layout;

		m_WireframePipeline = Pipeline::Create(pipeline_spec);

		PrimitiveMeshGenerator mesh_generator;
		MeshPreprocessor mesh_preprocessor;

		// Icosphere
		auto icosphere_data = mesh_generator.GenerateIcosphere(2);
		std::vector<glm::vec3> icosphere_vertices = mesh_preprocessor.ConvertToLineTopology(mesh_preprocessor.RemapVertices(icosphere_data.first, icosphere_data.second));

		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.buffer_usage = DeviceBufferUsage::VERTEX_BUFFER;
		buffer_spec.heap = DeviceBufferMemoryHeap::DEVICE;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.size = icosphere_vertices.size() * sizeof glm::vec3;

		m_IcosphereMesh = DeviceBuffer::Create(buffer_spec, icosphere_vertices.data(), buffer_spec.size);

		// Cube
		auto cube_data = mesh_generator.GenerateCube();
		std::vector<glm::vec3> cube_vertices = mesh_preprocessor.ConvertToLineTopology(mesh_preprocessor.RemapVertices(cube_data.first, cube_data.second));

		buffer_spec.size = cube_vertices.size() * sizeof glm::vec3;

		m_CubeMesh = DeviceBuffer::Create(buffer_spec, cube_vertices.data(), buffer_spec.size);
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

			PushConstants* pc_data = new PushConstants;
			memset(pc_data, 0u, sizeof PushConstants);
			pc_data->camera_data_bda = renderer->m_CameraBuffer->GetDeviceAddress() + renderer->m_CameraBuffer->GetFrameOffset();
			pc_data->trs = trs;
			pc_data->lines_color = encoded_color;
			
			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof PushConstants;

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

			PushConstants* pc_data = new PushConstants;
			memset(pc_data, 0u, sizeof PushConstants);
			pc_data->camera_data_bda = renderer->m_CameraBuffer->GetDeviceAddress() + renderer->m_CameraBuffer->GetFrameOffset();
			pc_data->trs = trs;
			pc_data->lines_color = encoded_color;

			MiscData pcs = {};
			pcs.data = (byte*)pc_data;
			pcs.size = sizeof PushConstants;

			Renderer::RenderUnindexed(renderer->m_WireframePipeline, renderer->m_CubeMesh, pcs);
		});
	};

	void DebugRenderer::Render(Shared<Image> target)
	{
		Renderer::BeginRender({ target }, target->GetSpecification().extent, { 0,0 }, { 0.0f, 0.0f, 0.0f, 0.0f });

		for (auto& request : renderer->m_DebugRequests)
			request();

		Renderer::EndRender(target);

		renderer->m_DebugRequests.clear();
	}

}