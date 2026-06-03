#include <Haoyue.h>
#include <Haoyue/EntryPoint.h>

#include "EditorLayer.h"

#include "Haoyue/Renderer/RendererAPI.h"

class HaoyueEditorApplication : public Haoyue::Application
{
public:
	HaoyueEditorApplication(const Haoyue::ApplicationSpecification& specification)
		: Application(specification)
	{
	}

	virtual void OnInit() override
	{
		PushLayer(new Haoyue::EditorLayer());
	}
};

Haoyue::Application* Haoyue::CreateApplication(int argc, char** argv)
{
	Haoyue::ApplicationSpecification specification;
	specification.Name = "Haoyue-Editor";
	specification.WindowWidth = 1600;
	specification.WindowHeight = 900;
	specification.VSync = true;
	return new HaoyueEditorApplication(specification);
}
