#include <CursedEngine.h>

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
		
		if (Input::KeyPressed(KeyCode::KEY_SPACE)) {
			Renderer::ClearImage(swapchain_image, { 0.2f, 0.7f, 0.1f, 1.0f });
		}
		else {
			Renderer::ClearImage(swapchain_image, { 0.1f, 0.3f, 0.7f, 1.0f });
		}
	}

	void OnEvent(Event* e) override
	{

	}

	void Destroy() override
	{
		m_Buffer->Destroy();
	}

	void Launch() override
	{
		fvec4 arr[] = {
			{0.0f, 0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 0.0f},
			{0.0f, 1.0f, 1.0f, 0.0f},
			{1.0f, 0.0f, 1.0f, 1.0f},
		};

		DeviceBufferSpecification buffer_spec = {};
		buffer_spec.size = sizeof(arr);
		buffer_spec.buffer_usage = DeviceBufferUsage::VERTEX_BUFFER;
		buffer_spec.memory_usage = DeviceBufferMemoryUsage::NO_HOST_ACCESS;
		buffer_spec.flags = (uint64)DeviceBufferFlags::VERTEX_RATE;


		m_Buffer = DeviceBuffer::Create(buffer_spec, arr, sizeof(arr));
	}

	/*
	*	DATA
	*/

	Shared<DeviceBuffer> m_Buffer;

};

Subsystem* ConstructRootSystem() 
{
	return new RootSubsystem;
}