#include "pch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>

#include "Haoyue/Core/Application.h"
#include "Haoyue/Math/Math.h"
#include "Haoyue/Renderer/Mesh.h"
#include "Haoyue/Script/ScriptEngine.h"
#include "Haoyue/Physics/Physics.h"
#include "Haoyue/Physics/PhysicsActor.h"
#include "Haoyue/Physics/PhysicsLayer.h"
#include "Haoyue/Physics/PXPhysicsWrappers.h"
#include "Haoyue/Renderer/MeshFactory.h"

#include "Haoyue/Audio/AudioEngine.h"
#include "Haoyue/Audio/AudioComponent.h"

#include "Haoyue/Asset/AssetManager.h"

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Haoyue/ImGui/ImGui.h"
#include "Haoyue/Renderer/Renderer.h"
#include "Haoyue/Editor/TranslationManager.h"
#include "Haoyue/Editor/EditorResources.h"

namespace Haoyue {

	glm::mat4 Mat4FromAssimpMat4(const aiMatrix4x4& matrix);

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
		: m_Context(context)
	{
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_Context = scene;
		m_SelectionContext = {};
		if (m_SelectionContext && false)
		{
			// Try and find same entity in new scene
			auto& entityMap = m_Context->GetEntityMap();
			UUID selectedEntityID = m_SelectionContext.GetUUID();
			if (entityMap.find(selectedEntityID) != entityMap.end())
				m_SelectionContext = entityMap.at(selectedEntityID);
		}
	}

	void SceneHierarchyPanel::SetSelected(Entity entity)
	{
		m_SelectionContext = entity;

		if (m_SelectionChangedCallback)
			m_SelectionChangedCallback(m_SelectionContext);
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");
		ImRect windowRect = { ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax() };

		if (m_Context)
		{
			uint32_t entityCount = 0, meshCount = 0;
			m_Context->m_Registry.each([&](auto entity)
			{
				Entity e(entity, m_Context.Raw());
				if (e.HasComponent<IDComponent>() && e.GetParentUUID() == 0)
					DrawEntityNode(e);
			});

			if (ImGui::BeginDragDropTargetCustom(windowRect, ImGui::GetCurrentWindow()->ID))
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

				if (payload)
				{
					UUID droppedHandle = *((UUID*)payload->Data);
					Entity e = m_Context->FindEntityByUUID(droppedHandle);
					m_Context->UnparentEntity(e);
				}

				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::BeginMenu(TR("Create")))
				{
					if (ImGui::MenuItem(TR("Empty Entity")))
					{
						auto newEntity = m_Context->CreateEntity(TR("Empty Entity"));
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem(TR("Camera")))
					{
						auto newEntity = m_Context->CreateEntity(TR("Camera"));
						newEntity.AddComponent<CameraComponent>();
						SetSelected(newEntity);
					}
					if (ImGui::BeginMenu(TR("Mesh")))
					{
						if (ImGui::MenuItem(TR("Empty Mesh")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Empty Mesh"));
							newEntity.AddComponent<MeshComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem(TR("Cube")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Cube"));
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("Resources/meshes/Default/Cube.fbx"));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem(TR("Sphere")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Sphere"));
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("Resources/meshes/Default/Sphere.fbx"));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem(TR("Capsule")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Capsule"));
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("Resources/meshes/Default/Capsule.fbx"));
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem(TR("Plane")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Plane"));
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("Resources/meshes/Default/Plane.fbx"));
							SetSelected(newEntity);
						}
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu(TR("Physics")))
					{
						if (ImGui::MenuItem(TR("Rigidbody")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Rigidbody"));
							newEntity.AddComponent<RigidBodyComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem(TR("Box")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Cube"));
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("Resources/meshes/Default/Cube.fbx"));
							newEntity.AddComponent<BoxColliderComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem(TR("Sphere")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Sphere"));
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("Resources/meshes/Default/Sphere.fbx"));
							newEntity.AddComponent<SphereColliderComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem(TR("Capsule")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Capsule"));
							newEntity.AddComponent<MeshComponent>(AssetManager::GetAsset<Mesh>("Resources/meshes/Default/Capsule.fbx"));
							newEntity.AddComponent<CapsuleColliderComponent>();
							SetSelected(newEntity);
						}
						if (ImGui::MenuItem(TR("Mesh")))
						{
							auto newEntity = m_Context->CreateEntity(TR("Capsule"));
							newEntity.AddComponent<MeshComponent>();
							newEntity.AddComponent<MeshColliderComponent>();
							SetSelected(newEntity);
						}
						ImGui::EndMenu();
					}
					ImGui::Separator();
					if (ImGui::MenuItem(TR("Directional Light")))
					{
						auto newEntity = m_Context->CreateEntity(TR("Directional Light"));
						newEntity.AddComponent<DirectionalLightComponent>();
						newEntity.GetComponent<TransformComponent>().Rotation = glm::radians(glm::vec3{ 80.0f, 10.0f, 0.0f });
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem(TR("Sky Light")))
					{
						auto newEntity = m_Context->CreateEntity(TR("Sky Light"));
						newEntity.AddComponent<SkyLightComponent>();
						SetSelected(newEntity);
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			ImGui::End();

			ImGui::Begin("Properties");

			if (m_SelectionContext)
				DrawComponents(m_SelectionContext);
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const char* name = TR("Unnamed Entity");
		if (entity.HasComponent<TagComponent>())
			name = entity.GetComponent<TagComponent>().Tag.c_str();

		ImGuiTreeNodeFlags flags = (entity == m_SelectionContext ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		if (entity.Children().empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		// TODO(Peter): This should probably be a function that checks that the entities components are valid
		bool missingMesh = entity.HasComponent<MeshComponent>() && (entity.GetComponent<MeshComponent>().Mesh && entity.GetComponent<MeshComponent>().Mesh->IsFlagSet(AssetFlag::Missing));
		if (missingMesh)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.3f, 1.0f));

		bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, name);
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
			if (m_SelectionChangedCallback)
				m_SelectionChangedCallback(m_SelectionContext);
		}

		if (missingMesh)
			ImGui::PopStyleColor();

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem(TR("Delete")))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			UUID entityId = entity.GetUUID();
			ImGui::Text(entity.GetComponent<TagComponent>().Tag.c_str());
			ImGui::SetDragDropPayload("scene_entity_hierarchy", &entityId, sizeof(UUID));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene_entity_hierarchy", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

			if (payload)
			{
				UUID droppedHandle = *((UUID*)payload->Data);
				Entity e = m_Context->FindEntityByUUID(droppedHandle);
				m_Context->ParentEntity(e, entity);
			}

			ImGui::EndDragDropTarget();
		}

		if (opened)
		{
			for (auto child : entity.Children())
			{
				Entity e = m_Context->FindEntityByUUID(child);
				if (e)
					DrawEntityNode(e);
			}

			ImGui::TreePop();
		}

		// Defer deletion until end of node UI
		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (entity == m_SelectionContext)
				m_SelectionContext = {};

			m_EntityDeletedCallback(entity);
		}
	}

	static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition(const glm::mat4& transform)
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		return { translation, orientation, scale };
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(Ref<Texture2D> icon, const std::string& name, Entity entity, UIFunction uiFunction, bool canBeRemoved = true)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			// NOTE(Peter):
			//	This fixes an issue where the first "+" button would display the "Remove" buttons for ALL components on an Entity.
			//	This is due to ImGui::TreeNodeEx only pushing the id for it's children if it's actually open
			ImGui::PushID((void*)typeid(T).hash_code());
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();

			ImGui::SameLine();
			UI::ImageButton(icon, ImVec2{ lineHeight, lineHeight - 2.0f }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 }, ImGui::GetColorU32(ImGuiCol_ButtonHovered));

			ImGui::SameLine(0.0f, 3.0f);
			bool open = ImGui::TreeNodeEx("##dummy_id", treeNodeFlags, TR(name.c_str()));
			bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
			ImGui::PopStyleVar();

			bool resetValues = false;
			bool removeComponent = false;

			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }) || right_clicked)
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Reset"))
					resetValues = true;

				if (canBeRemoved)
				{
					if (ImGui::MenuItem("Remove component"))
						removeComponent = true;
				}

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent || resetValues)
				entity.RemoveComponent<T>();

			if (resetValues)
				entity.AddComponent<T>();

			ImGui::PopID();
		}
	}

	static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		bool modified = false;

		const ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
		{
			values.x = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
		{
			values.y = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
		{
			values.z = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		return modified;
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		ImGui::AlignTextToFramePadding();

		auto id = entity.GetComponent<IDComponent>().ID;

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			char buffer[256];
			memset(buffer, 0, 256);
			memcpy(buffer, tag.c_str(), tag.length());
			ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);
			if (ImGui::InputText("##Tag", buffer, 256))
			{
				tag = std::string(buffer);
			}
			ImGui::PopItemWidth();
		}

		// ID
		ImGui::SameLine();
		ImGui::TextDisabled("%llx", id);
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 textSize = ImGui::CalcTextSize(TR("Add Component"));
		ImGui::SameLine(contentRegionAvailable.x - (textSize.x + GImGui->Style.FramePadding.y));
		if (ImGui::Button(TR("Add Component")))
			ImGui::OpenPopup("AddComponentPanel");

		if (ImGui::BeginPopup("AddComponentPanel"))
		{
			DisplayAddComponentEntry<CameraComponent>("Camera");
			DisplayAddComponentEntry<MeshComponent>("Mesh");
			DisplayAddComponentEntry<DirectionalLightComponent>("Directional Light");
			DisplayAddComponentEntry<SkyLightComponent>("Sky Light");
			DisplayAddComponentEntry<ScriptComponent>("Script");
			DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
			DisplayAddComponentEntry<RigidBody2DComponent>("Rigidbody 2D");
			DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D");
			DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");
			DisplayAddComponentEntry<RigidBodyComponent>("Rigidbody");
			DisplayAddComponentEntry<BoxColliderComponent>("Box Collider");
			DisplayAddComponentEntry<SphereColliderComponent>("Sphere Collider");
			DisplayAddComponentEntry<CapsuleColliderComponent>("Capsule Collider");
			DisplayAddComponentEntry<Audio::AudioComponent>("Audio");
			if (!m_SelectionContext.HasComponent<MeshColliderComponent>())
			{
				if (ImGui::MenuItem("Mesh Collider"))
				{
					MeshColliderComponent& component = m_SelectionContext.AddComponent<MeshColliderComponent>();
					if (m_SelectionContext.HasComponent<MeshComponent>())
					{
						component.CollisionMesh = m_SelectionContext.GetComponent<MeshComponent>().Mesh;
						PXPhysicsWrappers::CreateTriangleMesh(component);
					}

					ImGui::CloseCurrentPopup();
				}
			}

			if (!m_SelectionContext.HasComponent<AudioListenerComponent>())
			{
				if (ImGui::MenuItem("Audio Listener"))
				{
					auto view = m_Context->GetAllEntitiesWith<AudioListenerComponent>();
					bool listenerExists = !view.empty();
					auto& listenerComponent = m_SelectionContext.AddComponent<AudioListenerComponent>();

					listenerComponent.Active = !listenerExists;
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}

		DrawComponent<TransformComponent>(EditorResources::TransformIcon, "Transform", entity, [](TransformComponent& component)
		{
			DrawVec3Control(TR("Translation"), component.Translation);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control(TR("Rotation"), rotation);
			component.Rotation = glm::radians(rotation);
			DrawVec3Control(TR("Scale"), component.Scale, 1.0f);
		}, false);

		DrawComponent<MeshComponent>(EditorResources::ComponentIcon, "Mesh", entity, [&](MeshComponent& mc)
		{
			UI::BeginPropertyGrid();
			if (UI::PropertyAssetReference(TR("Mesh"), mc.Mesh))
			{
				if (entity.HasComponent<MeshColliderComponent>())
				{
					auto& mcc = entity.GetComponent<MeshColliderComponent>();
					mcc.CollisionMesh = mc.Mesh;
					if (mcc.IsConvex)
						PXPhysicsWrappers::CreateConvexMesh(mcc, entity.Transform().Scale, true);
					else
						PXPhysicsWrappers::CreateTriangleMesh(mcc, entity.Transform().Scale, true);
				}
			}
			UI::EndPropertyGrid();
		});

		DrawComponent<CameraComponent>(EditorResources::CameraIcon, "Camera", entity, [](CameraComponent& cc)
		{
			UI::BeginPropertyGrid();

			// Projection Type
			const char* projTypeStrings[] = { TR("Perspective"), TR("Orthographic") };
			int currentProj = (int)cc.Camera.GetProjectionType();
			if (UI::PropertyDropdown(TR("Projection"), projTypeStrings, 2, &currentProj))
			{
				cc.Camera.SetProjectionType((SceneCamera::ProjectionType)currentProj);
			}

			// Perspective parameters
			if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float verticalFOV = cc.Camera.GetPerspectiveVerticalFOV();
				if (UI::Property(TR("Vertical FOV"), verticalFOV))
					cc.Camera.SetPerspectiveVerticalFOV(verticalFOV);

				float nearClip = cc.Camera.GetPerspectiveNearClip();
				if (UI::Property(TR("Near Clip"), nearClip))
					cc.Camera.SetPerspectiveNearClip(nearClip);
				ImGui::SameLine();
				float farClip = cc.Camera.GetPerspectiveFarClip();
				if (UI::Property(TR("Far Clip"), farClip))
					cc.Camera.SetPerspectiveFarClip(farClip);
			}

			// Orthographic parameters
			else if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = cc.Camera.GetOrthographicSize();
				if (UI::Property(TR("Size"), orthoSize))
					cc.Camera.SetOrthographicSize(orthoSize);

				float nearClip = cc.Camera.GetOrthographicNearClip();
				if (UI::Property(TR("Near Clip"), nearClip))
					cc.Camera.SetOrthographicNearClip(nearClip);
				ImGui::SameLine();
				float farClip = cc.Camera.GetOrthographicFarClip();
				if (UI::Property(TR("Far Clip"), farClip))
					cc.Camera.SetOrthographicFarClip(farClip);
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<SpriteRendererComponent>(EditorResources::RendererIcon, "Sprite Renderer", entity, [](SpriteRendererComponent& mc)
		{
		});

		DrawComponent<DirectionalLightComponent>(EditorResources::ComponentIcon, "Directional Light", entity, [](DirectionalLightComponent& dlc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyColor(TR("Radiance"), dlc.Radiance);
			UI::Property(TR("Intensity"), dlc.Intensity);
			UI::Property(TR("Cast Shadows"), dlc.CastShadows);
			UI::Property(TR("Soft Shadows"), dlc.SoftShadows);
			UI::Property(TR("Source Size"), dlc.LightSize);
			UI::EndPropertyGrid();
		});

		DrawComponent<SkyLightComponent>(EditorResources::ComponentIcon, "Sky Light", entity, [](SkyLightComponent& slc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyAssetReference(TR("Environment Map"), slc.SceneEnvironment);
			UI::Property(TR("Intensity"), slc.Intensity, 0.01f, 0.0f, 5.0f);
			ImGui::Separator();
			UI::Property(TR("Dynamic Sky"), slc.DynamicSky);
			if (slc.DynamicSky)
			{
				bool changed = UI::Property(TR("Turbidity"), slc.TurbidityAzimuthInclination.x, 0.01f);
				changed |= UI::Property(TR("Azimuth"), slc.TurbidityAzimuthInclination.y, 0.01f);
				changed |= UI::Property(TR("Inclination"), slc.TurbidityAzimuthInclination.z, 0.01f);
				if (changed)
				{
					Ref<TextureCube> preethamEnv = Renderer::CreatePreethamSky(slc.TurbidityAzimuthInclination.x, slc.TurbidityAzimuthInclination.y, slc.TurbidityAzimuthInclination.z);
					slc.SceneEnvironment = Ref<Environment>::Create(preethamEnv, preethamEnv);
				}
			}
			UI::EndPropertyGrid();
		});

		DrawComponent<ScriptComponent>(EditorResources::ScriptIcon, "Script", entity, [=](ScriptComponent& sc) mutable
		{
			UI::BeginPropertyGrid();
			std::string oldName = sc.ModuleName;
			if (UI::Property(TR("Module Name"), sc.ModuleName, !ScriptEngine::ModuleExists(sc.ModuleName))) // TODO: no live edit
			{
				// Shutdown old script
				if (ScriptEngine::ModuleExists(oldName))
					ScriptEngine::ShutdownScriptEntity(entity, oldName);

				if (ScriptEngine::ModuleExists(sc.ModuleName))
					ScriptEngine::InitScriptEntity(entity);
			}

			// Public Fields
			if (ScriptEngine::ModuleExists(sc.ModuleName))
			{
				EntityInstanceData& entityInstanceData = ScriptEngine::GetEntityInstanceData(entity.GetSceneUUID(), id);
				auto& moduleFieldMap = entityInstanceData.ModuleFieldMap;
				if (moduleFieldMap.find(sc.ModuleName) != moduleFieldMap.end())
				{
					auto& publicFields = moduleFieldMap.at(sc.ModuleName);
					for (auto& [name, field] : publicFields)
					{
						bool isRuntime = m_Context->m_IsPlaying && field.IsRuntimeAvailable();
						switch (field.Type)
						{
						case FieldType::Int:
						{
							int value = isRuntime ? field.GetRuntimeValue<int>() : field.GetStoredValue<int>();
							if (UI::Property(field.Name.c_str(), value))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Float:
						{
							float value = isRuntime ? field.GetRuntimeValue<float>() : field.GetStoredValue<float>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Vec2:
						{
							glm::vec2 value = isRuntime ? field.GetRuntimeValue<glm::vec2>() : field.GetStoredValue<glm::vec2>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Vec3:
						{
							glm::vec3 value = isRuntime ? field.GetRuntimeValue<glm::vec3>() : field.GetStoredValue<glm::vec3>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Vec4:
						{
							glm::vec4 value = isRuntime ? field.GetRuntimeValue<glm::vec4>() : field.GetStoredValue<glm::vec4>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						}
					}
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<RigidBody2DComponent>(EditorResources::RigidBodyIcon, "Rigidbody 2D", entity, [](RigidBody2DComponent& rb2dc)
		{
			UI::BeginPropertyGrid();

			// Rigidbody2D Type
			const char* rb2dTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
			UI::PropertyDropdown("Type", rb2dTypeStrings, 3, (int*)&rb2dc.BodyType);

			if (rb2dc.BodyType == RigidBody2DComponent::Type::Dynamic)
			{
				UI::BeginPropertyGrid();
				UI::Property("Fixed Rotation", rb2dc.FixedRotation);
				UI::EndPropertyGrid();
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<BoxCollider2DComponent>(EditorResources::BoxColliderIcon, "Box Collider 2D", entity, [](BoxCollider2DComponent& bc2dc)
		{
			UI::BeginPropertyGrid();

			UI::Property("Offset", bc2dc.Offset);
			UI::Property("Size", bc2dc.Size);
			UI::Property("Density", bc2dc.Density);
			UI::Property("Friction", bc2dc.Friction);

			UI::EndPropertyGrid();
		});
	
		DrawComponent<CircleCollider2DComponent>(EditorResources::RendererIcon, "Circle Collider 2D", entity, [](CircleCollider2DComponent& cc2dc)
		{
			UI::BeginPropertyGrid();

			UI::Property("Offset", cc2dc.Offset);
			UI::Property("Radius", cc2dc.Radius);
			UI::Property("Density", cc2dc.Density);
			UI::Property("Friction", cc2dc.Friction);

			UI::EndPropertyGrid();
		});

		DrawComponent<RigidBodyComponent>(EditorResources::RigidBodyIcon, "Rigidbody", entity, [](RigidBodyComponent& rbc)
		{
			UI::BeginPropertyGrid();

			// Rigidbody Type
			const char* rbTypeStrings[] = { "Static", "Dynamic" };
			UI::PropertyDropdown("Type", rbTypeStrings, 2, (int*)&rbc.BodyType);

			// Layer has been removed, set to Default layer
			if (!PhysicsLayerManager::IsLayerValid(rbc.Layer))
				rbc.Layer = 0;

			int layerCount = PhysicsLayerManager::GetLayerCount();
			const auto& layerNames = PhysicsLayerManager::GetLayerNames();
			UI::PropertyDropdown("Layer", layerNames, layerCount, (int*)&rbc.Layer);

			if (rbc.BodyType == RigidBodyComponent::Type::Dynamic)
			{
				UI::BeginPropertyGrid();
				UI::Property("Mass", rbc.Mass);
				UI::Property("Linear Drag", rbc.LinearDrag);
				UI::Property("Angular Drag", rbc.AngularDrag);
				UI::Property("Disable Gravity", rbc.DisableGravity);
				UI::Property("Is Kinematic", rbc.IsKinematic);
				UI::EndPropertyGrid();

				if (UI::BeginTreeNode("Constraints", false))
				{
					UI::BeginPropertyGrid();

					UI::BeginCheckboxGroup("Freeze Position");
					UI::PropertyCheckboxGroup("X", rbc.LockPositionX);
					UI::PropertyCheckboxGroup("Y", rbc.LockPositionY);
					UI::PropertyCheckboxGroup("Z", rbc.LockPositionZ);
					UI::EndCheckboxGroup();

					UI::BeginCheckboxGroup("Freeze Rotation");
					UI::PropertyCheckboxGroup("X", rbc.LockRotationX);
					UI::PropertyCheckboxGroup("Y", rbc.LockRotationY);
					UI::PropertyCheckboxGroup("Z", rbc.LockRotationZ);
					UI::EndCheckboxGroup();
					
					UI::EndPropertyGrid();
					UI::EndTreeNode();
				}
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<BoxColliderComponent>(EditorResources::BoxColliderIcon, "Box Collider", entity, [](BoxColliderComponent& bcc)
		{
			UI::BeginPropertyGrid();

			if (UI::Property("Size", bcc.Size))
				bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);

			//Property("Offset", bcc.Offset);
			UI::Property("Is Trigger", bcc.IsTrigger);
			UI::PropertyAssetReference("Material", bcc.Material);

			UI::EndPropertyGrid();
		});

		DrawComponent<SphereColliderComponent>(EditorResources::ComponentIcon, "Sphere Collider", entity, [](SphereColliderComponent& scc)
		{
			UI::BeginPropertyGrid();

			if (UI::Property("Radius", scc.Radius))
			{
				scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
			}

			UI::Property("Is Trigger", scc.IsTrigger);
			UI::PropertyAssetReference("Material", scc.Material);

			UI::EndPropertyGrid();
		});

		DrawComponent<CapsuleColliderComponent>(EditorResources::ComponentIcon, "Capsule Collider", entity, [=](CapsuleColliderComponent& ccc)
		{
			UI::BeginPropertyGrid();

			bool changed = false;

			if (UI::Property("Radius", ccc.Radius))
				changed = true;

			if (UI::Property("Height", ccc.Height))
				changed = true;

			UI::Property("Is Trigger", ccc.IsTrigger);
			UI::PropertyAssetReference("Material", ccc.Material);

			if (changed)
			{
				ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<MeshColliderComponent>(EditorResources::ComponentIcon, "Mesh Collider", entity, [&](MeshColliderComponent& mcc)
		{
			UI::BeginPropertyGrid();

			if (mcc.OverrideMesh)
			{
				if (UI::PropertyAssetReference("Mesh", mcc.CollisionMesh))
				{
					if (mcc.IsConvex)
						PXPhysicsWrappers::CreateConvexMesh(mcc, entity.Transform().Scale, true);
					else
						PXPhysicsWrappers::CreateTriangleMesh(mcc, entity.Transform().Scale, true);
				}
			}

			if (UI::Property("Is Convex", mcc.IsConvex))
			{
				if (mcc.IsConvex)
					PXPhysicsWrappers::CreateConvexMesh(mcc, entity.Transform().Scale, true);
				else
					PXPhysicsWrappers::CreateTriangleMesh(mcc, entity.Transform().Scale, true);
			}

			UI::Property("Is Trigger", mcc.IsTrigger);
			UI::PropertyAssetReference("Material", mcc.Material);

			if (UI::Property("Override Mesh", mcc.OverrideMesh))
			{
				if (!mcc.OverrideMesh && entity.HasComponent<MeshComponent>())
				{
					mcc.CollisionMesh = entity.GetComponent<MeshComponent>().Mesh;

					if (mcc.IsConvex)
						PXPhysicsWrappers::CreateConvexMesh(mcc, entity.Transform().Scale, true);
					else
						PXPhysicsWrappers::CreateTriangleMesh(mcc, entity.Transform().Scale, true);
				}
			}
			UI::EndPropertyGrid();
		});

		DrawComponent<Audio::AudioComponent>(EditorResources::AudioIcon, "Audio", entity, [&](Audio::AudioComponent& ac)
		{
			// PropertyGrid consists out of 2 columns, so need to move cursor accordingly
			auto propertyGridSpacing = []
				{
					ImGui::Spacing();
					ImGui::NextColumn();
					ImGui::NextColumn();
				};
			auto singleColumnSeparator = []
				{
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					ImVec2 p = ImGui::GetCursorScreenPos();
					draw_list->AddLine(ImVec2(p.x - 9999, p.y), ImVec2(p.x + 9999, p.y), ImGui::GetColorU32(ImGuiCol_Border));
				};

			// Making separators a little bit less bright to "separate" them visually from the text
			auto& colors = ImGui::GetStyle().Colors;
			auto oldSCol = colors[ImGuiCol_Separator];
			const float brM = 0.6f;
			colors[ImGuiCol_Separator] = ImVec4{ oldSCol.x * brM, oldSCol.y * brM, oldSCol.z * brM, 1.0f };

			//=======================================================

			auto& soundConfig = ac.SoundConfig;

			// Adding space after header
			ImGui::Spacing();

			//--- Sound Assets and Looping
			//----------------------------
			UI::PushID();
			UI::BeginPropertyGrid();
			// Need to wrap this first Property Grid into another ID,
			// otherwise there's a conflict with the next Property Grid.

			bool bWasEmpty = soundConfig.FileAsset == nullptr;
			if (UI::PropertyAssetReference("Sound", soundConfig.FileAsset))
			{
				if (bWasEmpty)
					soundConfig.FileAsset.Create();
			}

			propertyGridSpacing();

			if (UI::Property("Volume Multiplier", soundConfig.VolumeMultiplier, 0.01f, 0.0f, 1.0f)) //TODO: switch to dBs in the future ?
			{
				ac.VolumeMultiplier = soundConfig.VolumeMultiplier;
			}
			if (UI::Property("Pitch Multiplier", soundConfig.PitchMultiplier, 0.01f, 0.0f, 24.0f)) // max pitch 24 is just an arbitrary number here
			{
				ac.PitchMultiplier = soundConfig.PitchMultiplier;
			}

			propertyGridSpacing();


			UI::Property("Play on Awake", ac.bPlayOnAwake);
			UI::Property("Looping", soundConfig.bLooping);

			UI::EndPropertyGrid();
			UI::PopID();

			//--- Preview buttons
			//-------------------
			if (soundConfig.FileAsset != nullptr)
			{

				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				// could use "inline sound" for the preview
				// for now this playback controls are just for testing
				// in the future should make a proper preview player with waveform view

				const float space = 10.0f;
				const ImVec2 buttonSize{ 70.0f, 30.0f };
				ImGui::SetCursorPosX(ImGui::GetColumnWidth() - (buttonSize.x * 3));

				if (ImGui::Button("Play", buttonSize))
					AudioPlayback::Play(ac.ParentHandle);

				ImGui::SameLine(0.0f, space);
				if (ImGui::Button("Stop", buttonSize))
					AudioPlayback::StopActiveSound(ac.ParentHandle);

				ImGui::SameLine(0.0f, space);
				if (ImGui::Button("Pause", buttonSize))
					AudioPlayback::PauseActiveSound(ac.ParentHandle);

				ImGui::SetCursorPosX(0);
			}

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Spacing();

			//--- Enable Spatialization
			//-------------------------
			ImGui::Text("Spatialization");
			ImGui::SameLine(contentRegionAvailable.x - (ImGui::GetFrameHeight() + GImGui->Style.FramePadding.y));
			ImGui::Checkbox("##enabled", &soundConfig.bSpatializationEnabled);


			//--- Spatialization Settings
			//---------------------------
			if (soundConfig.bSpatializationEnabled)
			{
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::Spacing();

				using AttModel = Audio::AttenuationModel;

				auto& spatialConfig = soundConfig.Spatialization;

				auto getTextForModel = [&](AttModel model)
					{
						switch (model)
						{
						case AttModel::None:
							return "None";
						case AttModel::Inverse:
							return "Inverse";
						case AttModel::Linear:
							return "Linear";
						case AttModel::Exponential:
							return "Exponential";
						}
					};

				const auto& attenModStr = std::vector<std::string>{ getTextForModel(AttModel::None),
																	getTextForModel(AttModel::Inverse),
																	getTextForModel(AttModel::Linear),
																	getTextForModel(AttModel::Exponential) };

				UI::BeginPropertyGrid();

				int32_t selectedModel = static_cast<int32_t>(spatialConfig.AttenuationMod);
				if (UI::PropertyDropdown("Attenuaion Model", attenModStr, attenModStr.size(), &selectedModel))
				{
					spatialConfig.AttenuationMod = static_cast<AttModel>(selectedModel);
				}

				singleColumnSeparator();
				propertyGridSpacing();
				propertyGridSpacing();
				UI::Property("Min Gain", spatialConfig.MinGain, 0.01f, 0.0f, 1.0f);
				UI::Property("Max Gain", spatialConfig.MaxGain, 0.01f, 0.0f, 1.0f);
				UI::Property("Min Distance", spatialConfig.MinDistance, 1.00f, 0.0f, FLT_MAX);
				UI::Property("Max Distance", spatialConfig.MaxDistance, 1.00f, 0.0f, FLT_MAX);

				singleColumnSeparator();
				propertyGridSpacing();
				propertyGridSpacing();

				float inAngle = glm::degrees(spatialConfig.ConeInnerAngleInRadians);
				float outAngle = glm::degrees(spatialConfig.ConeOuterAngleInRadians);
				float outGain = spatialConfig.ConeOuterGain;

				//? Have to manually clamp here because UI::Property doesn't take flags to pass in ImGuiSliderFlags_ClampOnInput
				if (UI::Property("Cone Inner Angle", inAngle, 1.0f, 0.0f, 360.0f))
				{
					if (inAngle > 360.0f) inAngle = 360.0f;
					spatialConfig.ConeInnerAngleInRadians = glm::radians(inAngle);
				}
				if (UI::Property("Cone Outer Angle", outAngle, 1.0f, 0.0f, 360.0f))
				{
					if (outAngle > 360.0f) outAngle = 360.0f;
					spatialConfig.ConeOuterAngleInRadians = glm::radians(outAngle);
				}
				if (UI::Property("Cone Outer Gain", outGain, 0.01f, 0.0f, 1.0f))
				{
					if (outGain > 1.0f) outGain = 1.0f;
					spatialConfig.ConeOuterGain = outGain;
				}

				singleColumnSeparator();
				propertyGridSpacing();
				propertyGridSpacing();
				if (UI::Property("Doppler Factor", spatialConfig.DopplerFactor, 0.01f, 0.0f, 1.0f)) {}
				//if (UI::Property("Rolloff", spatialConfig.Rolloff, 0.01f, 0.0f, 1.0f)) {  }

				propertyGridSpacing();
				propertyGridSpacing();
				// TODO: air absorption filter is not hooked up yet
				//if (UI::Property("Air Absorption", spatialConfig.bAirAbsorptionEnabled)) {  }

				UI::EndPropertyGrid();
			}

			colors[ImGuiCol_Separator] = oldSCol;
		});

		DrawComponent<AudioListenerComponent>(EditorResources::ListenerIcon, "Audio Listener", entity, [&](AudioListenerComponent& alc)
		{
			UI::BeginPropertyGrid();

			if (UI::Property("Active", alc.Active))
			{
				auto view = m_Context->GetAllEntitiesWith<AudioListenerComponent>();
				if (alc.Active == true)
				{
					for (auto ent : view)
					{
						Entity e{ ent, m_Context.Raw() };
						e.GetComponent<AudioListenerComponent>().Active = ent == entity;
					}
				}
				else
				{
					// Fallback to using main camera as active listener
					// - in editor main camera is already the only allowed active listener (may change that in the future)
					// - in runtime it falls back to main camera in update loop if can't find other active listener
				}
			}

			float inAngle = glm::degrees(alc.ConeInnerAngleInRadians);
			float outAngle = glm::degrees(alc.ConeOuterAngleInRadians);
			float outGain = alc.ConeOuterGain;
			//? Have to manually clamp here because UI::Property doesn't take flags to pass in ImGuiSliderFlags_ClampOnInput
			if (UI::Property("Inner Cone Angle", inAngle, 1.0f, 0.0f, 360.0f))
			{
				if (inAngle > 360.0f) inAngle = 360.0f;
				alc.ConeInnerAngleInRadians = glm::radians(inAngle);
			}
			if (UI::Property("Outer Cone Angle", outAngle, 1.0f, 0.0f, 360.0f))
			{
				if (outAngle > 360.0f) outAngle = 360.0f;
				alc.ConeOuterAngleInRadians = glm::radians(outAngle);
			}
			if (UI::Property("Outer Gain", outGain, 0.01f, 0.0f, 1.0f))
			{
				if (outGain > 1.0f) outGain = 1.0f;
				alc.ConeOuterGain = outGain;
			}
			UI::EndPropertyGrid();
		});
	}

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName) {
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(TR(entryName.c_str())))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

}
