#pragma once

#ifdef HY_PLATFORM_WINDOWS

extern Haoyue::Application* Haoyue::CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

int main(int argc, char** argv)
{
	while (g_ApplicationRunning)
	{
		Haoyue::InitializeCore();
		Haoyue::Application* app = Haoyue::CreateApplication(argc, argv);
		HY_CORE_ASSERT(app, "Client Application is null!");
		app->Run();
		delete app;
		Haoyue::ShutdownCore();
	}
}

#endif
