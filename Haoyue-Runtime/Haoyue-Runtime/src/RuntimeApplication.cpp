#include <Haoyue.h>
#include <Haoyue/EntryPoint.h>

#include "RuntimeLayer.h"

#include "Haoyue/Renderer/RendererAPI.h"

class RuntimeApplication : public Haoyue::Application
{
public:
	RuntimeApplication(const Haoyue::ApplicationSpecification& specification)
		: Application(specification)
	{
	}

	virtual void OnInit() override
	{
		PushLayer(new Haoyue::RuntimeLayer());
	}
};

Haoyue::Application* Haoyue::CreateApplication(int argc, char** argv)
{
	Haoyue::ApplicationSpecification specification;
	specification.Name = "Haoyue Runtime";
	specification.WindowWidth = 1600;
	specification.WindowHeight = 900;
	specification.VSync = true;
	specification.EnableImGui = false;
	specification.WorkingDirectory = "../Haoyue-Editor";
	return new RuntimeApplication(specification);
}
