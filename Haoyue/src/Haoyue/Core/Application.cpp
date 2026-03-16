#include "pch.h"
#include "Application.h"

#include "Haoyue/Renderer/Renderer.h"
#include "Haoyue/Renderer/Framebuffer.h"
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>

#include "Haoyue/Script/ScriptEngine.h"
#include "Haoyue/Physics/Physics.h"
#include "Haoyue/Asset/AssetManager.h"

#include "Input.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>

#include "Haoyue/Vulkan/VulkanRenderer.h"
#include "Haoyue/Vulkan/VulkanAllocator.h"

#include "Haoyue/Editor/TranslationManager.h"

extern bool g_ApplicationRunning;

namespace Haoyue {

#define BIND_EVENT_FN(fn) std::bind(&Application::##fn, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationProps& props)
	{
		s_Instance = this;

		m_Profiler = new PerformanceProfiler();

		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps(props.Name, props.WindowWidth, props.WindowHeight)));
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
		m_Window->Maximize();
		m_Window->SetVSync(true);

		// Init renderer and execute command queue to compile all shaders
		Renderer::Init();
		Renderer::WaitAndRender();
		
		m_ImGuiLayer = ImGuiLayer::Create();
		PushOverlay(m_ImGuiLayer);

		ScriptEngine::Init("Resources/scripts/ExampleApp.dll");
		Physics::Init();

		AssetManager::Init();
		TranslationManager::Init();
	}

	Application::~Application()
	{
		for (Layer* layer : m_LayerStack)
		{
			layer->OnDetach();
			delete layer;
		}

		FramebufferPool::GetGlobal()->GetAll().clear();
		
		Physics::Shutdown();
		ScriptEngine::Shutdown();
		TranslationManager::Shutdown();
		AssetManager::Shutdown();

		Renderer::WaitAndRender();
		Renderer::Shutdown();

		delete m_Profiler;
		m_Profiler = nullptr;
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::RenderImGui()
	{
		m_ImGuiLayer->Begin();
		ImGui::Begin(TR("Renderer"));
		{
			auto& caps = Renderer::GetCapabilities();
			ImGui::Text(TR("Vendor: %s"), caps.Vendor.c_str());
			ImGui::Text(TR("Renderer: %s"), caps.Device.c_str());
			ImGui::Text(TR("Version: %s"), caps.Version.c_str());
			ImGui::Separator();
			ImGui::Text(TR("Frame Time: %.2fms"), m_TimeStep.GetMilliseconds());

			if (RendererAPI::Current() == RendererAPIType::Vulkan)
			{
				GPUMemoryStats memoryStats = VulkanAllocator::GetStats();
				std::string used = Utils::BytesToString(memoryStats.Used);
				std::string free = Utils::BytesToString(memoryStats.Free);
				ImGui::Text(TR("Used VRAM: %s"), used.c_str());
				ImGui::Text(TR("Free VRAM: %s"), free.c_str());
			}
		}
		ImGui::End();

		ImGui::Begin(TR("Performance"));
		{
			ImGui::Text(TR("Frame Time: %.2fms"), m_TimeStep.GetMilliseconds());
			const auto& perFrameData = m_Profiler->GetPerFrameData();
			for (auto&& [name, time] : perFrameData)
			{
				ImGui::Text(TR("%s: %.3fms"), name, time);
			}
		}
		ImGui::End();
		m_Profiler->Clear();

		for (Layer* layer : m_LayerStack)
			layer->OnImGuiRender();
	}

	void Application::Run()
	{
		OnInit();
		while (m_Running)
		{
			static uint64_t frameCounter = 0;
			//HY_CORE_INFO("-- BEGIN FRAME {0}", frameCounter);
			m_Window->ProcessEvents();

			if (!m_Minimized)
			{
				Renderer::BeginFrame();
				//VulkanRenderer::BeginFrame();
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(m_TimeStep);
			
				// Render ImGui on render thread
				Application* app = this;
				Renderer::Submit([app]() { app->RenderImGui(); });
				Renderer::Submit([=]() {m_ImGuiLayer->End(); });
				Renderer::EndFrame();

				// On Render thread
				m_Window->GetRenderContext()->BeginFrame();
				Renderer::WaitAndRender();
				m_Window->SwapBuffers();
			}

			float time = GetTime();
			m_TimeStep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			//HY_CORE_INFO("-- END FRAME {0}", frameCounter);
			frameCounter++;
		}
		OnShutdown();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(event);
			if (event.Handled)
				break;
		}
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		int width = e.GetWidth(), height = e.GetHeight();	
		if (width == 0 || height == 0)
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;
		
		m_Window->GetRenderContext()->OnResize(width, height);

		auto& fbs = FramebufferPool::GetGlobal()->GetAll();
		for (auto& fb : fbs)
		{
			if (!fb->GetSpecification().NoResize)
				fb->Resize(width, height);
		}

		return false;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		g_ApplicationRunning = false; // Request close
		return true;
	}

	std::string Application::OpenFile(const char* filter) const
	{
		OPENFILENAMEA ofn;       // common dialog box structure
		CHAR szFile[260] = { 0 };       // if using TCHAR macros

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)m_Window->GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return std::string();
	}

	std::string Application::SaveFile(const char* filter) const
	{
		OPENFILENAMEA ofn;       // common dialog box structure
		CHAR szFile[260] = { 0 };       // if using TCHAR macros

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)m_Window->GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetSaveFileNameA(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return std::string();
	}

	float Application::GetTime() const
	{
		return (float)glfwGetTime();
	}

	const char* Application::GetConfigurationName()
	{
#if defined(HY_DEBUG)
		return "Debug";
#elif defined(HY_RELEASE)
		return "Release";
#elif defined(HY_DIST)
		return "Dist";
#else
	#error Undefined configuration?
#endif
	}

	const char* Application::GetPlatformName()
	{
#if defined(HY_PLATFORM_WINDOWS)
		return "Windows x64";
#else
	#error Undefined platform?
#endif
	}

}
