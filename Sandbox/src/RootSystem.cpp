#include <CursedEngine.h>

using namespace Cursed;

#include <functional>

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
		CURSED_CLIENT_TRACE("Exiting root system...");
	}

	void Launch() override
	{
		CURSED_CLIENT_TRACE("Constructed root system...");
	}

};

Subsystem* ConstructRootSystem() 
{
	return new RootSubsystem;
}