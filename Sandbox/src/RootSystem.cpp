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

	}

	void OnEvent(Event* e) override
	{
		
	}

	void Destroy() override
	{
		Renderer::WaitDevice();
		m_VertexBuffer->Destroy();
		m_IndexBuffer->Destroy();
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

		ShaderLibrary::Get()->Load("assets/shaders/test.ofs");
	}

	/*
	*	DATA
	*/

	Shared<DeviceBuffer> m_VertexBuffer;
	Shared<DeviceBuffer> m_IndexBuffer;
};

Subsystem* ConstructRootSystem() 
{
	return new RootSubsystem;
}