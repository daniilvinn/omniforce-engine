#include <Omniforce.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

using namespace Omni;

class RootSubsystem : public Subsystem 
{
public:
	~RootSubsystem() override 
	{
		Destroy();
	}

	void OnUpdate(float32 step) override
	{

	}

	void OnEvent(Event* e) override
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(OMNIFORCE_BIND_EVENT_FUNCTION(OnWindowResize));
	}

	bool OnWindowResize(WindowResizeEvent* e) {
		
		return false;
	}

	void Destroy() override
	{
	}

	void Launch() override
	{
	
	}
	
};

Subsystem* ConstructRootSystem() 
{
	return new RootSubsystem;
}