#include "pch.h"
#include "ObjectsPanel.h"
#include "Haoyue/ImGui/ImGui.h"
#include "Haoyue/Editor/EditorResources.h"

namespace Haoyue {

	ObjectsPanel::ObjectsPanel()
	{
	}

	void ObjectsPanel::DrawObject(const char* label, AssetHandle handle)
	{
		UI::Image(EditorResources::AssetIcon, ImVec2(30, 30));
		ImGui::SameLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
		ImGui::Selectable(label);

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			UI::Image(EditorResources::AssetIcon, ImVec2(20, 20));
			ImGui::SameLine();

			ImGui::Text(label);

			ImGui::SetDragDropPayload("asset_payload", &handle, sizeof(AssetHandle));
			ImGui::EndDragDropSource();
		}
	}

	void ObjectsPanel::OnImGuiRender()
	{
		static const AssetHandle CubeHandle = AssetManager::GetAssetHandleFromFilePath("Resources/meshes/Default/Cube.fbx");
		static const AssetHandle CapsuleHandle = AssetManager::GetAssetHandleFromFilePath("Resources/meshes/Default/Capsule.fbx");
		static const AssetHandle SphereHandle = AssetManager::GetAssetHandleFromFilePath("Resources/meshes/Default/Sphere.fbx");
		static const AssetHandle CylinderHandle = AssetManager::GetAssetHandleFromFilePath("Resources/meshes/Default/Cylinder.fbx");
		static const AssetHandle TorusHandle = AssetManager::GetAssetHandleFromFilePath("Resources/meshes/Default/Torus.fbx");
		static const AssetHandle PlaneHandle = AssetManager::GetAssetHandleFromFilePath("Resources/meshes/Default/Plane.fbx");
		static const AssetHandle ConeHandle = AssetManager::GetAssetHandleFromFilePath("Resources/meshes/Default/Cone.fbx");

		ImGui::Begin("Objects");
		{
			ImGui::BeginChild("##objects_window");
			DrawObject("Cube", CubeHandle);
			DrawObject("Capsule", CapsuleHandle);
			DrawObject("Sphere", SphereHandle);
			DrawObject("Cylinder", CylinderHandle);
			DrawObject("Torus", TorusHandle);
			DrawObject("Plane", PlaneHandle);
			DrawObject("Cone", ConeHandle);
			ImGui::EndChild();
		}

		ImGui::End();
	}

}