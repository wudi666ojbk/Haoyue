#include <Haoyue.h>
#include <Haoyue/EntryPoint.h>

#include "EditorLayer.h"

#include "Haoyue/Renderer/RendererAPI.h"

class HaoYuenutApplication : public Haoyue::Application
{
public:
	HaoYuenutApplication(const Haoyue::ApplicationProps& props)
		: Application(props)
	{
	}

	virtual void OnInit() override
	{
		PushLayer(new Haoyue::EditorLayer());
	}
};

Haoyue::Application* Haoyue::CreateApplication(int argc, char** argv)
{
	return new HaoYuenutApplication({"Haoyue-Editor", 1600, 900});
}
