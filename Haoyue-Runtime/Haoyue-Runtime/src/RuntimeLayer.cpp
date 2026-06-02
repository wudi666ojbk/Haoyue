#include "RuntimeLayer.h"

#include "Haoyue/ImGui/ImGuizmo.h"
#include "Haoyue/Renderer/Renderer2D.h"
#include "Haoyue/Script/ScriptEngine.h"
#include "Panel/PhysicsSettingsWindow.h"
#include "Panel/AssetEditorPanel.h"

#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Haoyue/Physics/Physics.h"
#include "Haoyue/Math/Math.h"
#include "Haoyue/Utilities/FileSystem.h"

#include "Haoyue/Renderer/RendererAPI.h"

#include "imgui_internal.h"
#include "Haoyue/ImGui/ImGui.h"

namespace Haoyue {

	RuntimeLayer::RuntimeLayer()
		: m_EditorCamera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
	{
	}

	RuntimeLayer::~RuntimeLayer()
	{
	}

	void RuntimeLayer::OnAttach()
	{
		OpenScene("Resources/scenes/levels/Physics2D-Game.hsc");
		SceneRendererSpecification spec;
		spec.SwapChainTarget = true;
		m_SceneRenderer = Ref<SceneRenderer>::Create(m_RuntimeScene, spec);
		m_SceneRenderer->GetOptions().ShowGrid = false;
		m_SceneRenderer->SetShadowSettings(-50.0f, 50.0f, 0.95f);
		OnScenePlay();
	}

	void RuntimeLayer::OnDetach()
	{
	}

	void RuntimeLayer::OnScenePlay()
	{
		m_RuntimeScene->OnRuntimeStart();
	}

	void RuntimeLayer::OnSceneStop()
	{

	}

	void RuntimeLayer::UpdateWindowTitle(const std::string& sceneName)
	{
		Application::Get().GetWindow().SetTitle(sceneName);
	}

	void RuntimeLayer::OnUpdate(Timestep ts)
	{
		auto [width, height] = Application::Get().GetWindow().GetSize();
		m_SceneRenderer->SetViewportSize(width, height);
		m_RuntimeScene->SetViewportSize(width, height);

		if (m_ViewportPanelFocused)
			m_EditorCamera.OnUpdate(ts);

		m_RuntimeScene->OnUpdate(ts);
		m_RuntimeScene->OnRenderRuntime(m_SceneRenderer, ts);
	}

	void RuntimeLayer::ShowBoundingBoxes(bool show, bool onTop)
	{
		m_SceneRenderer->GetOptions().ShowBoundingBoxes = show && !onTop;
		m_DrawOnTopBoundingBoxes = show && onTop;
	}

	void RuntimeLayer::OpenScene(const std::string& filepath)
	{
		Ref<Scene> newScene = Ref<Scene>::Create("New Scene", false);
		SceneSerializer serializer(newScene);
		serializer.Deserialize(filepath);
		m_RuntimeScene = newScene;
		m_SceneFilePath = filepath;

		std::filesystem::path path = filepath;
		UpdateWindowTitle(path.filename().string());
		ScriptEngine::SetSceneContext(m_RuntimeScene);
	}

	void RuntimeLayer::OnEvent(Event& e)
	{
		m_RuntimeScene->OnEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(HY_BIND_EVENT_FN(RuntimeLayer::OnKeyPressedEvent));
		dispatcher.Dispatch<MouseButtonPressedEvent>(HY_BIND_EVENT_FN(RuntimeLayer::OnMouseButtonPressed));
	}

	bool RuntimeLayer::OnKeyPressedEvent(KeyPressedEvent& e)
	{

		switch (e.GetKeyCode())
		{
		case KeyCode::Escape:
			break;
		}

		return false;
	}

	bool RuntimeLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{

		return false;
	}

}
