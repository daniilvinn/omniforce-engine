#include <Omniforce.h>

using namespace Omni;

class RootSubsystem : public Subsystem 
{
public:
	~RootSubsystem() override 
	{
		Destroy();
	}

	void OnUpdate() override
	{
		Shared<Image> swapchain_image = Renderer::GetSwapchainImage();
		
		Renderer::ClearImage(swapchain_image, { 0.420, 0.69, 0.14 });

		if (Input::KeyPressed(KeyCode::KEY_SPACE)) {
			ShaderLibrary::Get()->Unload("basic.vert");
		}

		if (ShaderLibrary::Get()->Has("basic.vert")) {
			Renderer::ClearImage(swapchain_image, { 0.123, 0.321, 0.420 });
		};

		Renderer::BeginRender(swapchain_image, swapchain_image->GetSpecification().extent, { 0,0 }, { 0.0f, 0.0f, 1.0f });
		Renderer::RenderMesh(m_Pipeline, m_VertexBuffer, m_IndexBuffer);
		Renderer::EndRender(swapchain_image);
	}

	void OnEvent(Event* e) override
	{

	}

	void Destroy() override
	{
		Renderer::WaitDevice();
		m_VertexBuffer->Destroy();
		m_IndexBuffer->Destroy();
		m_Pipeline->Destroy();
	}

	void Launch() override
	{
		float vertices[] = {
			-0.5f, -0.5f,	1.0f, 0.0f, 0.0f,
			 0.5f, -0.5f,	0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f,	0.0f, 0.0f, 1.0f,
			-0.5f,  0.5f,	1.0f, 0.0f, 0.0f
		};

		uint8 indices[] = {
			0, 1, 2,
			2, 3, 0
		};

		DeviceBufferSpecification vertex_buffer_spec = {};
		vertex_buffer_spec.size = sizeof(vertices);
		vertex_buffer_spec.buffer_usage = DeviceBufferUsage::VERTEX_BUFFER;
		vertex_buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		vertex_buffer_spec.flags = (uint64)DeviceBufferFlags::VERTEX_RATE;

		DeviceBufferSpecification index_buffer_spec = {};
		index_buffer_spec.size = sizeof(indices);
		index_buffer_spec.buffer_usage = DeviceBufferUsage::INDEX_BUFFER;
		index_buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		index_buffer_spec.flags = (uint64)DeviceBufferFlags::INDEX_TYPE_UINT8;

		m_VertexBuffer = DeviceBuffer::Create(vertex_buffer_spec, vertices, sizeof(vertices));
		m_IndexBuffer = DeviceBuffer::Create(index_buffer_spec, indices, sizeof(indices));

		JobSystem::Get()->Wait();

		ShaderProgram program({
			ShaderLibrary::Get()->Get("basic.vert"),
			ShaderLibrary::Get()->Get("color_pass.frag"),
		});

		DeviceBufferLayout input_layout({
			{ "pos", DeviceDataType::FLOAT2 },
			{ "color", DeviceDataType::FLOAT3 }
		});

		PipelineSpecification pipeline_spec = PipelineSpecification::Default();
		pipeline_spec.debug_name = "test pipeline";
		pipeline_spec.input_layout = input_layout;
		pipeline_spec.program = program;
		pipeline_spec.output_attachments_formats = { ImageFormat::BGRA32 };
	
		m_Pipeline = Pipeline::Create(pipeline_spec);

		ShaderLibrary::Get()->Unload("basic.vert");
		ShaderLibrary::Get()->Unload("color_pass.frag");
	}

	/*
	*	DATA
	*/

	Shared<DeviceBuffer> m_VertexBuffer;
	Shared<DeviceBuffer> m_IndexBuffer;
	Shared<Pipeline> m_Pipeline;
};

Subsystem* ConstructRootSystem() 
{
	return new RootSubsystem;
}