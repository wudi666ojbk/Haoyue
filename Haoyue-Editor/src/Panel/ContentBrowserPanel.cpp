#include "pch.h"
#include "ContentBrowserPanel.h"
#include "Haoyue/Core/Application.h"
#include "AssetEditorPanel.h"
#include "Haoyue/Core/Input.h"

#include "Haoyue/Editor/EditorResources.h"
#include <filesystem>
#include <imgui_internal.h>

namespace Haoyue {

	ContentBrowserPanel::ContentBrowserPanel()
	{
		AssetManager::SetAssetChangeCallback(HY_BIND_EVENT_FN(ContentBrowserPanel::OnFileSystemChanged));

		m_BaseDirectoryHandle = ProcessDirectory("Resources", 0);

		m_AssetIconMap[""]		= EditorResources::FolderIcon;
		m_AssetIconMap["fbx"]	= EditorResources::FbxIcon;
		m_AssetIconMap["obj"]	= EditorResources::ObjIcon;
		m_AssetIconMap["wav"]	= EditorResources::WavIcon;
		m_AssetIconMap["cs"]	= EditorResources::CscIcon;
		m_AssetIconMap["png"]	= EditorResources::PngIcon;
		m_AssetIconMap["hsc"]	= EditorResources::MoonIcon;

		m_BaseDirectory = m_Directories[m_BaseDirectoryHandle];
		UpdateCurrentDirectory(m_BaseDirectoryHandle);

		memset(m_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
		memset(m_SearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	AssetHandle ContentBrowserPanel::ProcessDirectory(const std::string& directoryPath, AssetHandle parent)
	{
		Ref<DirectoryInfo> directoryInfo = Ref<DirectoryInfo>::Create();
		directoryInfo->Handle = AssetHandle();
		directoryInfo->Parent = parent;
		directoryInfo->FilePath = directoryPath;
		std::replace(directoryInfo->FilePath.begin(), directoryInfo->FilePath.end(), '\\', '/');
		directoryInfo->Name = Utils::GetFilename(directoryPath);

		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
			{
				directoryInfo->SubDirectories.push_back(ProcessDirectory(entry.path().string(), directoryInfo->Handle));
				continue;
			}

			const auto& metadata = AssetManager::GetMetadata(entry.path().string());

			if (!metadata.IsValid())
				continue;

			directoryInfo->Assets.push_back(metadata.Handle);
		}

		m_Directories[directoryInfo->Handle] = directoryInfo;
		return directoryInfo->Handle;
	}

	void ContentBrowserPanel::DrawDirectoryInfo(AssetHandle directory)
	{
		const auto& dir = m_Directories[directory];

		if (ImGui::TreeNode(dir->Name.c_str()))
		{
			for (AssetHandle child : dir->SubDirectories)
				DrawDirectoryInfo(child);
			ImGui::TreePop();
		}

		// Only works when TreeNode is open and doesn't have any child TreeNodes. Most likely a bug with ImGui
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			UpdateCurrentDirectory(directory);
	}

	static int s_ColumnCount = 10;
	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser", NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
		{
			UI::BeginPropertyGrid();
			ImGui::SetColumnOffset(1, 240);

			ImGui::BeginChild("##folders_common");
			{
				if (ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (AssetHandle child : m_BaseDirectory->SubDirectories)
					{
						DrawDirectoryInfo(child);
					}
				}
			}
			ImGui::EndChild();

			ImGui::NextColumn();

			ImGui::BeginChild("##directory_structure", ImVec2(0, ImGui::GetWindowHeight() - 65));
			{
				ImGui::BeginChild("##top_bar", ImVec2(0, 30));
				{
					RenderBreadCrumbs();
				}
				ImGui::EndChild();

				ImGui::Separator();

				ImGui::BeginChild("Scrolling");
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.35f));

					if (Input::IsKeyPressed(KeyCode::Escape) || (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_IsAnyItemHovered))
					{
						m_SelectedAssets.Clear();
						m_RenamingSelected = false;
						memset(m_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
					}

					m_IsAnyItemHovered = false;

					if (ImGui::BeginPopupContextWindow(0, 1, false))
					{
						if (ImGui::BeginMenu("Create"))
						{
							if (ImGui::MenuItem("Folder"))
							{
								bool created = FileSystem::CreateFolder(m_CurrentDirectory->FilePath + "/New Folder");

								if (created)
								{
									UpdateCurrentDirectory(m_CurrentDirHandle);
									auto& createdDirectory = GetDirectoryInfo(m_CurrentDirectory->FilePath + "/New Folder");
									SelectAndStartRenaming(createdDirectory->Handle, createdDirectory->Name);
								}
							}

							if (ImGui::MenuItem("Physics Material"))
							{
								auto asset = CreateAsset<PhysicsMaterial>("New Physics Material.hpm", 0.6f, 0.6f, 0.0f);
								const auto& metadata = AssetManager::GetMetadata(asset->Handle);
								SelectAndStartRenaming(metadata.Handle, metadata.FileName);
							}

							ImGui::EndMenu();
						}

						if (ImGui::MenuItem("Import"))
						{
							std::string filepath = Application::Get().OpenFile();
							if (!filepath.empty())
							{
								AssetHandle handle = AssetManager::ImportAsset(filepath);
								if (handle != 0)
								{
									m_CurrentDirectory->Assets.push_back(handle);
									UpdateCurrentDirectory(m_CurrentDirHandle);
								}
							}
						}

						if (ImGui::MenuItem("Refresh"))
						{
							UpdateCurrentDirectory(m_CurrentDirHandle);
						}

						ImGui::EndPopup();
					}

					ImGui::Columns(s_ColumnCount, nullptr, false);

					for (auto& directory : m_CurrentDirectories)
					{
						RenderDirectory(directory);
						ImGui::NextColumn();
					}

					for (auto& asset : m_CurrentAssets)
					{
						RenderAsset(asset);
						ImGui::NextColumn();
					}

					if (m_UpdateDirectoryNextFrame)
					{
						UpdateCurrentDirectory(m_CurrentDirHandle);
						m_UpdateDirectoryNextFrame = false;
					}

					if (m_IsDragging && !ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.1f))
					{
						m_IsDragging = false;
					}

					ImGui::PopStyleColor(2);
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();

			RenderBottomBar();

			UI::EndPropertyGrid();
		}
		ImGui::End();
	}

	void ContentBrowserPanel::RenderDirectory(Ref<DirectoryInfo>& directory)
	{
		ImGui::PushID(&directory->Handle);
		ImGui::BeginGroup();

		bool selected = m_SelectedAssets.IsSelected(directory->Handle);

		if (selected)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));

		float buttonWidth = ImGui::GetColumnWidth() - 15.0f;
		UI::ImageButton(directory->FilePath.c_str(), EditorResources::FolderIcon->GetImage(), {buttonWidth, buttonWidth});

		if (selected)
			ImGui::PopStyleColor();

		HandleDragDrop(EditorResources::FolderIcon->GetImage(), directory->Handle);

		if (ImGui::IsItemHovered())
		{
			m_IsAnyItemHovered = true;

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				m_PrevDirHandle = m_CurrentDirHandle;
				m_CurrentDirHandle = directory->Handle;
				m_UpdateDirectoryNextFrame = true;
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_IsDragging)
			{
				if (!Input::IsKeyPressed(KeyCode::LeftControl))
					m_SelectedAssets.Clear();

				if (selected)
					m_SelectedAssets.Deselect(directory->Handle);
				else
					m_SelectedAssets.Select(directory->Handle);
			}
		}

		bool openDeleteModal = false;

		// TODO: Delete multiple items at once
		if (selected && Input::IsKeyPressed(KeyCode::Delete) && !openDeleteModal && m_SelectedAssets.SelectionCount() == 1)
		{
			openDeleteModal = true;
		}

		if (ImGui::BeginPopupContextItem("ContextMenu"))
		{
			if (ImGui::MenuItem("Rename"))
			{
				m_SelectedAssets.Select(directory->Handle);
				memset(m_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
				memcpy(m_RenameBuffer, directory->Name.c_str(), directory->Name.size());
				m_RenamingSelected = true;
			}

			if (ImGui::MenuItem("Delete"))
				openDeleteModal = true;

			ImGui::EndPopup();
		}

		if (openDeleteModal)
		{
			ImGui::OpenPopup("Delete Asset");
			openDeleteModal = false;
		}

		bool deleted = false;
		if (ImGui::BeginPopupModal("Delete Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to delete %s and everything within it?", directory->Name.c_str());

			float columnWidth = ImGui::GetContentRegionAvailWidth() / 4;

			ImGui::Columns(4, 0, false);
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::SetColumnWidth(1, columnWidth);
			ImGui::SetColumnWidth(2, columnWidth);
			ImGui::SetColumnWidth(3, columnWidth);
			ImGui::NextColumn();
			if (ImGui::Button("Yes", ImVec2(columnWidth, 0)))
			{
				AssetHandle handle = directory->Handle;
				std::string filepath = directory->FilePath;
				deleted = FileSystem::DeleteFile(filepath);

				if (deleted)
				{
					m_Directories.erase(handle);
					m_CurrentDirectory->SubDirectories.erase(std::remove(m_CurrentDirectory->SubDirectories.begin(), m_CurrentDirectory->SubDirectories.end(), handle), m_CurrentDirectory->SubDirectories.end());
					m_UpdateDirectoryNextFrame = true;
				}

				ImGui::CloseCurrentPopup();
			}

			ImGui::NextColumn();
			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("No", ImVec2(columnWidth, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::NextColumn();
			ImGui::EndPopup();
		}

		if (!deleted)
		{
			ImGui::SetNextItemWidth(buttonWidth);

			if (!selected || !m_RenamingSelected)
				ImGui::TextWrapped(directory->Name.c_str());

			if (selected)
				HandleDirectoryRename(directory);
		}

		ImGui::EndGroup();
		ImGui::PopID();
	}

	void ContentBrowserPanel::RenderAsset(AssetMetadata& assetInfo)
	{
		ImGui::PushID(&assetInfo.Handle);
		ImGui::BeginGroup();

		Ref<Image2D> iconRef = m_AssetIconMap.find(assetInfo.Extension) != m_AssetIconMap.end() ? m_AssetIconMap[assetInfo.Extension]->GetImage() : EditorResources::FileIcon->GetImage();

		bool selected = m_SelectedAssets.IsSelected(assetInfo.Handle);
		if (selected)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));

		float buttonWidth = ImGui::GetColumnWidth() - 15.0f;
		UI::ImageButton(assetInfo.FilePath.c_str(), iconRef, { buttonWidth, buttonWidth });
		if (selected)
			ImGui::PopStyleColor();

		HandleDragDrop(iconRef, assetInfo.Handle);

		if (ImGui::IsItemHovered())
		{
			m_IsAnyItemHovered = true;

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (assetInfo.Type == AssetType::Scene)
				{
					// TODO: Open scene in viewport
				}
				else
				{
					AssetEditorPanel::OpenEditor(AssetManager::GetAsset<Asset>(assetInfo.Handle));
				}
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_IsDragging)
			{
				if (!Input::IsKeyPressed(KeyCode::LeftControl))
					m_SelectedAssets.Clear();

				if (selected)
					m_SelectedAssets.Deselect(assetInfo.Handle);
				else
					m_SelectedAssets.Select(assetInfo.Handle);
			}
		}

		bool openDeleteModal = false;

		// TODO: Delete multiple items at once
		if (selected && Input::IsKeyPressed(KeyCode::Delete) && !openDeleteModal && m_SelectedAssets.SelectionCount() == 1)
		{
			openDeleteModal = true;
		}

		if (ImGui::BeginPopupContextItem("ContextMenu"))
		{
			ImGui::Text(assetInfo.FilePath.c_str());

			ImGui::Separator();

			if (ImGui::MenuItem("Rename"))
			{
				m_SelectedAssets.Select(assetInfo.Handle);
				memset(m_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
				memcpy(m_RenameBuffer, assetInfo.FileName.c_str(), assetInfo.FileName.size());
				m_RenamingSelected = true;
			}

			if (ImGui::MenuItem("Delete"))
				openDeleteModal = true;

			ImGui::EndPopup();
		}

		if (openDeleteModal)
		{
			ImGui::OpenPopup("Delete Asset");
			openDeleteModal = false;
		}

		bool deleted = false;
		if (ImGui::BeginPopupModal("Delete Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to delete %s?", assetInfo.FileName.c_str());

			float columnWidth = ImGui::GetContentRegionAvailWidth() / 4;

			ImGui::Columns(4, 0, false);
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::SetColumnWidth(1, columnWidth);
			ImGui::SetColumnWidth(2, columnWidth);
			ImGui::SetColumnWidth(3, columnWidth);
			ImGui::NextColumn();
			if (ImGui::Button("Yes", ImVec2(columnWidth, 0)))
			{
				// Cache this so that we can delete the meta file if the asset was deleted successfully
				deleted = FileSystem::DeleteFile(assetInfo.FilePath);
				if (deleted)
				{
					m_CurrentDirectory->Assets.erase(std::remove(m_CurrentDirectory->Assets.begin(), m_CurrentDirectory->Assets.end(), assetInfo.Handle), m_CurrentDirectory->Assets.end());
					AssetManager::RemoveAsset(assetInfo.Handle);
					m_UpdateDirectoryNextFrame = true;
				}

				ImGui::CloseCurrentPopup();
			}

			ImGui::NextColumn();
			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("No", ImVec2(columnWidth, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::NextColumn();
			ImGui::EndPopup();
		}

		if (!deleted)
		{
			ImGui::SetNextItemWidth(buttonWidth);

			if (!selected || !m_RenamingSelected)
				ImGui::TextWrapped(assetInfo.FileName.c_str());

			if (selected)
				HandleAssetRename(assetInfo);
		}

		ImGui::EndGroup();
		ImGui::PopID();
	}

	void ContentBrowserPanel::HandleDragDrop(Ref<Image2D> icon, AssetHandle assetHandle)
	{
		if (m_Directories.find(assetHandle) != m_Directories.end() && m_IsDragging)
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto payload = ImGui::AcceptDragDropPayload("asset_payload");
				if (payload)
				{
					int count = payload->DataSize / sizeof(AssetHandle);

					for (int i = 0; i < count; i++)
					{
						AssetHandle handle = *(((AssetHandle*)payload->Data) + i);
						auto& directory = m_Directories[assetHandle];

						if (m_Directories.find(handle) == m_Directories.end())
						{
							bool wasMoved = AssetManager::MoveAsset(handle, directory->FilePath);
							if (!wasMoved)
								continue;

							auto previousDirectory = GetDirectoryForAsset(handle);
							previousDirectory->Assets.erase(std::remove(previousDirectory->Assets.begin(), previousDirectory->Assets.end(), handle), previousDirectory->Assets.end());
							directory->Assets.push_back(handle);
						}
						else
						{
							MoveDirectory(handle, directory->FilePath);
						}
					}

					m_UpdateDirectoryNextFrame = true;
				}

				ImGui::EndDragDropTarget();
			}
		}

		if (!m_SelectedAssets.IsSelected(assetHandle) || m_IsDragging)
			return;

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped) && ImGui::IsItemClicked(ImGuiMouseButton_Left))
			m_IsDragging = true;

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			UI::Image(icon, ImVec2(20, 20));
			ImGui::SameLine();
			if (m_SelectedAssets.SelectionCount() == 1)
			{
				if (AssetManager::IsAssetHandleValid(m_SelectedAssets[0]))
					ImGui::Text(AssetManager::GetMetadata(m_SelectedAssets[0]).FileName.c_str());
				else
					ImGui::Text(m_Directories[m_SelectedAssets[0]]->Name.c_str());
			}
			else
			{
				ImGui::Text("Dragging %d items", m_SelectedAssets.SelectionCount());
			}

			ImGui::SetDragDropPayload("asset_payload", m_SelectedAssets.GetSelectionData(), m_SelectedAssets.SelectionCount() * sizeof(AssetHandle));
			m_IsDragging = true;
			ImGui::EndDragDropSource();
		}
	}
	void ContentBrowserPanel::RenderBreadCrumbs()
	{
		if (UI::ImageButton(EditorResources::BackButtonIcon->GetImage(), ImVec2(20, 18)))
		{
			if (m_CurrentDirHandle == m_BaseDirectoryHandle) return;
			m_NextDirHandle = m_CurrentDirHandle;
			m_PrevDirHandle = m_CurrentDirectory->Parent;
			UpdateCurrentDirectory(m_PrevDirHandle);
		}

		ImGui::SameLine();

		if (UI::ImageButton(EditorResources::ForwardButtonIcon->GetImage(), ImVec2(20, 18)))
			UpdateCurrentDirectory(m_NextDirHandle);

		ImGui::SameLine();

		{
			ImGui::PushItemWidth(200);
			if (ImGui::InputTextWithHint("", "Search...", m_SearchBuffer, MAX_INPUT_BUFFER_LENGTH))
			{
				if (strlen(m_SearchBuffer) == 0)
				{
					UpdateCurrentDirectory(m_CurrentDirHandle);
				}
				else
				{
					SearchResults results = Search(m_SearchBuffer, m_CurrentDirHandle);
					m_CurrentDirectories = results.Directories;
					m_CurrentAssets = results.Assets;
				}
			}

			ImGui::PopItemWidth();
		}

		ImGui::SameLine();

		if (m_UpdateBreadCrumbs)
		{
			m_BreadCrumbData.clear();

			AssetHandle currentHandle = m_CurrentDirHandle;
			while (currentHandle != 0)
			{
				Ref<DirectoryInfo>& dirInfo = m_Directories[currentHandle];
				m_BreadCrumbData.push_back(dirInfo);
				currentHandle = dirInfo->Parent;
			}

			std::reverse(m_BreadCrumbData.begin(), m_BreadCrumbData.end());

			m_UpdateBreadCrumbs = false;
		}

		for (const auto& directory : m_BreadCrumbData)
		{
			if (directory->Name != "Resources")
				ImGui::Text("/");

			ImGui::SameLine();

			ImVec2 textSize = ImGui::CalcTextSize(directory->Name.c_str());
			if (ImGui::Selectable(directory->Name.c_str(), false, 0, ImVec2(textSize.x, 22)))
			{
				UpdateCurrentDirectory(directory->Handle);
			}

			ImGui::SameLine();
		}
	}

	void ContentBrowserPanel::RenderBottomBar()
	{
		ImGui::BeginChild("##panel_controls", ImVec2(ImGui::GetColumnWidth() - 12, 30), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			ImGui::Separator();

			ImGui::Columns(4, 0, false);

			if (m_SelectedAssets.SelectionCount() == 1)
			{
				const AssetMetadata& asset = AssetManager::GetMetadata(m_SelectedAssets[0]);

				if (asset.IsValid())
					ImGui::Text(asset.FilePath.c_str());
				else
					ImGui::Text(m_Directories[m_SelectedAssets[0]]->FilePath.c_str());
			}
			else if (m_SelectedAssets.SelectionCount() > 1)
			{
				ImGui::Text("%d items selected", m_SelectedAssets.SelectionCount());
			}

			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
			ImGui::SliderInt("##column_count", &s_ColumnCount, 2, 15);
		}
		ImGui::EndChild();
	}

	void ContentBrowserPanel::SelectAndStartRenaming(AssetHandle handle, const std::string& currentName)
	{
		m_SelectedAssets.Select(handle);
		memset(m_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
		memcpy(m_RenameBuffer, currentName.c_str(), currentName.size());
		m_RenamingSelected = true;
	}

	void ContentBrowserPanel::HandleDirectoryRename(Ref<DirectoryInfo>& dirInfo)
	{
		if (m_SelectedAssets.SelectionCount() > 1)
			return;

		if (!m_RenamingSelected && Input::IsKeyPressed(KeyCode::F2))
		{
			memset(m_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
			memcpy(m_RenameBuffer, dirInfo->Name.c_str(), dirInfo->Name.size());
			m_RenamingSelected = true;
		}

		if (m_RenamingSelected)
		{
			ImGui::SetKeyboardFocusHere();
			if (ImGui::InputText("##rename_dummy", m_RenameBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				std::string newFilePath = FileSystem::Rename(dirInfo->FilePath, m_RenameBuffer);
				dirInfo->FilePath = newFilePath;
				dirInfo->Name = Utils::GetFilename(newFilePath);
				m_RenamingSelected = false;
				m_SelectedAssets.Clear();
				m_UpdateDirectoryNextFrame = true;
			}
		}
	}

	void ContentBrowserPanel::HandleAssetRename(AssetMetadata& asset)
	{
		if (m_SelectedAssets.SelectionCount() > 1)
			return;

		if (!m_RenamingSelected && Input::IsKeyPressed(KeyCode::F2))
		{
			memset(m_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
			memcpy(m_RenameBuffer, asset.FileName.c_str(), asset.FileName.size());
			m_RenamingSelected = true;
		}

		if (m_RenamingSelected)
		{
			ImGui::SetKeyboardFocusHere();
			if (ImGui::InputText("##rename_dummy", m_RenameBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				AssetManager::Rename(asset.Handle, m_RenameBuffer);
				m_RenamingSelected = false;
				m_SelectedAssets.Clear();
				m_UpdateDirectoryNextFrame = true;
			}
		}
	}

	void ContentBrowserPanel::UpdateCurrentDirectory(AssetHandle directoryHandle)
	{
		m_CurrentDirectories.clear();
		m_CurrentAssets.clear();
		m_SelectedAssets.Clear();

		m_UpdateBreadCrumbs = true;
		m_CurrentDirHandle = directoryHandle;
		m_CurrentDirectory = m_Directories[directoryHandle];

		for (auto& assetHandle : m_CurrentDirectory->Assets)
			m_CurrentAssets.push_back(AssetManager::GetMetadata(assetHandle));

		for (auto& handle : m_CurrentDirectory->SubDirectories)
			m_CurrentDirectories.push_back(m_Directories[handle]);

		std::sort(m_CurrentDirectories.begin(), m_CurrentDirectories.end(), [](const Ref<DirectoryInfo>& a, const Ref<DirectoryInfo>& b)
			{
				return Utils::ToLower(a->Name) < Utils::ToLower(b->Name);
			});

		std::sort(m_CurrentAssets.begin(), m_CurrentAssets.end(), [](const AssetMetadata& a, const AssetMetadata& b)
			{
				return Utils::ToLower(a.FileName) < Utils::ToLower(b.FileName);
			});
	}

	void ContentBrowserPanel::OnFileSystemChanged(FileSystemChangedEvent e)
	{
		switch (e.Action)
		{
		case FileSystemAction::Added:
			if (e.IsDirectory)
				OnDirectoryAdded(e.FilePath);
			break;
		case FileSystemAction::Delete:
			OnAssetDeleted(e);
			break;
		case FileSystemAction::Modified:
			break;
		case FileSystemAction::Rename:
		{
			if (!e.IsDirectory)
				break;

			std::filesystem::path oldFilePath = e.FilePath;
			oldFilePath = oldFilePath.parent_path() / e.OldName;

			Ref<DirectoryInfo> dirInfo = GetDirectoryInfo(oldFilePath.string());
			dirInfo->FilePath = e.FilePath;
			dirInfo->Name = e.NewName;
			break;
		}
		}

		UpdateCurrentDirectory(m_CurrentDirHandle);
	}

	void ContentBrowserPanel::OnAssetDeleted(const FileSystemChangedEvent& e)
	{
		if (!e.IsDirectory)
		{
			AssetHandle handle = AssetManager::GetAssetHandleFromFilePath(e.FilePath);

			for (auto& [dirHandle, dirInfo] : m_Directories)
			{
				for (auto it = dirInfo->Assets.begin(); it != dirInfo->Assets.end(); it++)
				{
					if (*it != handle)
						continue;

					dirInfo->Assets.erase(it);
					return;
				}
			}
		}
		else
		{
			RemoveDirectory(GetDirectoryInfo(e.FilePath));
		}
	}

	void ContentBrowserPanel::RemoveDirectory(Ref<DirectoryInfo>& dirInfo)
	{
		if (dirInfo->Parent != 0)
		{
			auto& childList = m_Directories[dirInfo->Parent]->SubDirectories;
			childList.erase(std::remove(childList.begin(), childList.end(), dirInfo->Handle), childList.end());
		}

		for (auto subdir : dirInfo->SubDirectories)
			RemoveDirectory(m_Directories[subdir]);

		for (auto assetHandle : dirInfo->Assets)
			AssetManager::RemoveAsset(assetHandle);

		dirInfo->Assets.clear();
		dirInfo->SubDirectories.clear();

		m_Directories.erase(m_Directories.find(dirInfo->Handle));

		std::sort(m_CurrentDirectories.begin(), m_CurrentDirectories.end(), [](const Ref<DirectoryInfo>& a, const Ref<DirectoryInfo>& b)
			{
				return Utils::ToLower(a->Name) < Utils::ToLower(b->Name);
			});
	}

	void ContentBrowserPanel::OnDirectoryAdded(const std::string& directoryPath)
	{
		std::filesystem::path parentPath = directoryPath;
		parentPath = parentPath.parent_path();

		Ref<DirectoryInfo> parentInfo = GetDirectoryInfo(parentPath.string());

		if (parentInfo == nullptr)
		{
			HY_CORE_ERROR("This shouldn't happen...");
			return;
		}

		AssetHandle directoryHandle = ProcessDirectory(directoryPath, parentInfo->Handle);
		Ref<DirectoryInfo> directoryInfo = m_Directories[directoryHandle];
		parentInfo->SubDirectories.push_back(directoryHandle);

		for (auto& entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
			{
				OnDirectoryAdded(entry.path().string());
			}
			else
			{
				AssetHandle handle = AssetManager::ImportAsset(entry.path().string());
				directoryInfo->Assets.push_back(handle);
			}
		}

		std::sort(m_CurrentDirectories.begin(), m_CurrentDirectories.end(), [](const Ref<DirectoryInfo>& a, const Ref<DirectoryInfo>& b)
			{
				return Utils::ToLower(a->Name) < Utils::ToLower(b->Name);
			});
	}

	void ContentBrowserPanel::MoveDirectory(AssetHandle directoryHandle, const std::string& destinationPath)
	{
		auto& directoryInfo = m_Directories[directoryHandle];
		bool wasMoved = FileSystem::MoveFile(directoryInfo->FilePath, destinationPath);

		if (!wasMoved) return;

		UpdateDirectoryPath(directoryInfo, destinationPath);

		auto& parentDirectory = m_Directories[directoryInfo->Parent];
		parentDirectory->SubDirectories.erase(std::remove(parentDirectory->SubDirectories.begin(), parentDirectory->SubDirectories.end(), directoryHandle), parentDirectory->SubDirectories.end());

		auto& newParent = GetDirectoryInfo(destinationPath);
		newParent->SubDirectories.push_back(directoryHandle);
		directoryInfo->Parent = newParent->Handle;
	}

	void ContentBrowserPanel::UpdateDirectoryPath(Ref<DirectoryInfo>& directoryInfo, const std::string& newParentPath)
	{
		directoryInfo->FilePath = newParentPath + "/" + directoryInfo->Name;

		for (auto& subdir : directoryInfo->SubDirectories)
			UpdateDirectoryPath(m_Directories[subdir], directoryInfo->FilePath);
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectoryInfo(const std::string& filepath) const
	{
		std::string fixedFilepath = filepath;
		std::replace(fixedFilepath.begin(), fixedFilepath.end(), '\\', '/');

		for (auto& [handle, directoryInfo] : m_Directories)
		{
			if (directoryInfo->FilePath == fixedFilepath)
				return directoryInfo;
		}

		return nullptr;
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectoryForAsset(AssetHandle asset) const
	{
		for (const auto& [directoryHandle, directoryInfo] : m_Directories)
		{
			for (const auto& assetHandle : directoryInfo->Assets)
			{
				if (asset == assetHandle)
					return directoryInfo;
			}
		}

		return nullptr;
	}

	SearchResults ContentBrowserPanel::Search(const std::string& query, AssetHandle directoryHandle)
	{
		SearchResults results;

		std::string queryLowerCase = Utils::ToLower(query);

		const auto& directory = m_Directories[directoryHandle];

		for (auto& childHandle : directory->SubDirectories)
		{
			const auto& subdir = m_Directories[childHandle];

			if (subdir->Name.find(queryLowerCase) != std::string::npos)
				results.Directories.push_back(subdir);

			results.Append(Search(query, childHandle));
		}

		for (auto& assetHandle : directory->Assets)
		{
			const auto& asset = AssetManager::GetMetadata(assetHandle);

			std::string filename = Utils::ToLower(asset.FileName);

			if (filename.find(queryLowerCase) != std::string::npos)
				results.Assets.push_back(asset);

			if (queryLowerCase[0] != '.')
				continue;

			if (asset.Extension.find(std::string(&queryLowerCase[1])) != std::string::npos)
				results.Assets.push_back(asset);
		}

		return results;
	}

}
