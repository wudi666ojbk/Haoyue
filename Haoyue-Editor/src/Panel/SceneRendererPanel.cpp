#include "pch.h"
#include "SceneRendererPanel.h"

#include <imgui.h>
#include "Haoyue/ImGui/ImGui.h"
#include "Haoyue/Editor/TranslationManager.h"

namespace Haoyue {

	static const char* s_StylizedModeNames[] = {
		"None (PBR)",
		"Cartoon",
		"Pixelation",
		"Sketch",
		"Kuwahara"
	};
	static_assert(sizeof(s_StylizedModeNames) / sizeof(const char*) == 5, "StylizedMode enum mismatch");

	SceneRendererPanel::SceneRendererPanel(Ref<SceneRenderer> renderer)
		: m_Renderer(renderer)
	{
	}

	void SceneRendererPanel::OnImGuiRender()
	{
		auto& options = m_Renderer->GetOptions();

		ImGui::Begin("Scene Renderer");

		// === Shader reload ===
		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = Shader::s_AllShaders;
			for (auto& shader : shaders)
			{
				if (ImGui::TreeNode(shader->GetName().c_str()))
				{
					std::string buttonName = "Reload##" + shader->GetName();
					if (ImGui::Button(buttonName.c_str()))
						shader->Reload(true);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		// === Stylized effect selector (dropdown) ===
		{
			UI::BeginPropertyGrid();

			int32_t currentMode = static_cast<int32_t>(options.StylizedEffect);
			if (UI::PropertyDropdown("Stylized Effect", s_StylizedModeNames, 5, &currentMode))
			{
				options.StylizedEffect = static_cast<StylizedMode>(currentMode);
			}

			UI::EndPropertyGrid();
		}

		// === Conditional parameters based on selected effect ===
		switch (options.StylizedEffect)
		{
		case StylizedMode::Cartoon:
			if (UI::BeginTreeNode("Cartoon Settings", true))
			{
				UI::BeginPropertyGrid();
				UI::PropertySlider("Toon Levels", options.CartoonToonLevels, 2, 8);
				UI::Property("Specular Intensity", options.CartoonSpecularIntensity, 0.01f, 0.0f, 2.0f);
				UI::Property("Rim Light Intensity", options.CartoonRimLightIntensity, 0.01f, 0.0f, 2.0f);
				UI::PropertyColor("Outline Color", options.CartoonOutlineColor);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			break;

		case StylizedMode::Pixelation:
			if (UI::BeginTreeNode("Pixelation Settings", true))
			{
				UI::BeginPropertyGrid();
				UI::PropertySlider("Pixel Density", options.PixelDensity, 20.0f, 640.0f);
				UI::Property("Color Levels", options.PixelColorLevels, 1.0f, 0.0f, 32.0f);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			break;

		case StylizedMode::Sketch:
			if (UI::BeginTreeNode("Sketch Settings", true))
			{
				UI::BeginPropertyGrid();
				UI::PropertySlider("Hatch Density", options.HatchDensity, 4.0f, 30.0f);
				UI::Property("Hatch Intensity", options.HatchIntensity, 0.1f, 0.0f, 2.0f);
				UI::Property("Edge Strength", options.EdgeStrength, 0.1f, 0.0f, 2.0f);
				UI::PropertyColor("Ink Color", options.InkColor);
				UI::PropertyColor("Paper Color", options.PaperColor);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			break;

		case StylizedMode::Kuwahara:
			if (UI::BeginTreeNode("Kuwahara Settings", true))
			{
				UI::BeginPropertyGrid();
				UI::PropertySlider("Kernel Radius", options.KuwaharaRadius, 1.0f, 10.0f);
				UI::Property("Color Levels", options.KuwaharaColorLevels, 1.0f, 0.0f, 32.0f);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}
			break;

		default:
			break;
		}

		// === Shadows ===
		if (UI::BeginTreeNode("Shadows"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Soft Shadows", m_Renderer->RendererDataUB.SoftShadows);
			UI::Property("Light Size", m_Renderer->RendererDataUB.LightSize, 0.01f);
			UI::Property("Max Shadow Distance", m_Renderer->RendererDataUB.MaxShadowDistance, 1.0f);
			UI::Property("Shadow Fade", m_Renderer->RendererDataUB.ShadowFade, 5.0f);
			UI::EndPropertyGrid();

			if (UI::BeginTreeNode("Cascade Settings"))
			{
				UI::BeginPropertyGrid();
				UI::Property("Show Cascades", m_Renderer->RendererDataUB.ShowCascades);
				UI::Property("Cascade Fading", m_Renderer->RendererDataUB.CascadeFading);
				UI::Property("Cascade Transition Fade", m_Renderer->RendererDataUB.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
				UI::Property("Cascade Split", m_Renderer->CascadeSplitLambda, 0.01f);
				UI::Property("CascadeNearPlaneOffset", m_Renderer->CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
				UI::Property("CascadeFarPlaneOffset", m_Renderer->CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}

			if (UI::BeginTreeNode("Shadow Map", false))
			{
				static int cascadeIndex = 0;
				auto fb = m_Renderer->ShadowMapRenderPass[cascadeIndex]->GetSpecification().TargetFramebuffer;
				auto image = fb->GetDepthImage();

				float size = ImGui::GetContentRegionAvailWidth();
				UI::BeginPropertyGrid();
				UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
				UI::EndPropertyGrid();
				UI::Image(image, (uint32_t)cascadeIndex, { size, size }, { 0, 1 }, { 1, 0 });
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}

		ImGui::End();
	}

}
