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

	void OnEvent() override 
	{



	}

	void Destroy() override
	{
		printf("Exiting root system...\n");
	}

	void Launch() override
	{
		printf("Constructed root system...\n");
	}

};

Subsystem* ConstructRootSystem() 
{
	return new RootSubsystem;
}