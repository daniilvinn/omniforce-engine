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

		static fvec4 clear_value = { 0.0f, 0.0f, 0.0f, 0.0f };

		if (Input::KeyPressed(KeyCode::KEY_W))
			clear_value.r += 0.0003f;
		if (Input::KeyPressed(KeyCode::KEY_S))
			clear_value.r -= 0.0003f;
		if (Input::KeyPressed(KeyCode::KEY_A))
			clear_value.g -= 0.0003f;
		if (Input::KeyPressed(KeyCode::KEY_D))
			clear_value.g += 0.0003f;

		clear_value.r = std::clamp(clear_value.r, 0.0f, 1.0f);
		clear_value.g = std::clamp(clear_value.g, 0.0f, 1.0f);

		Renderer::BeginRender(swapchain_image, swapchain_image->GetSpecification().extent, { 0,0 }, clear_value);
		Renderer::RenderMesh(m_Pipeline, m_VertexBuffer, m_IndexBuffer, {});
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

		DeviceBufferLayout input_layout({
			{ "pos", DeviceDataType::FLOAT2 },
			{ "color", DeviceDataType::FLOAT3 }
		});

		PipelineSpecification pipeline_spec = PipelineSpecification::Default();
		pipeline_spec.debug_name = "test pipeline";
		pipeline_spec.input_layout = input_layout;
		pipeline_spec.shader = ShaderLibrary::Get()->GetShader("test.ofs");
		pipeline_spec.output_attachments_formats = { ImageFormat::BGRA32 };

		m_Pipeline = Pipeline::Create(pipeline_spec);

		ShaderLibrary::Get()->Unload("test.ofs");
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