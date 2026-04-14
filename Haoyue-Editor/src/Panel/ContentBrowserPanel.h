#pragma once

#include "Haoyue/Asset/AssetManager.h"
#include "Haoyue/Renderer/Texture.h"
#include "Haoyue/ImGui/ImGui.h"

#include <map>

#define MAX_INPUT_BUFFER_LENGTH 128

namespace Haoyue {

	struct SelectionStack
	{
	public:
		void Select(AssetHandle item)
		{
			m_Selections.push_back(item);
		}

		void Deselect(AssetHandle item)
		{
			for (auto it = m_Selections.begin(); it != m_Selections.end(); it++)
			{
				if (*it == item)
				{
					m_Selections.erase(it);
					break;
				}
			}
		}

		bool IsSelected(AssetHandle item) const
		{
			if (m_Selections.size() == 0)
				return false;

			for (auto selection : m_Selections)
			{
				if (selection == item)
					return true;
			}

			return false;
		}

		void Clear()
		{
			if (m_Selections.size() > 0)
				m_Selections.clear();
		}

		size_t SelectionCount() const
		{
			return m_Selections.size();
		}

		AssetHandle* GetSelectionData()
		{
			return m_Selections.data();
		}

		AssetHandle operator[](size_t index) const
		{
			HY_CORE_ASSERT(index >= 0 && index < m_Selections.size());
			return m_Selections[index];
		}

	private:
		std::vector<AssetHandle> m_Selections;
	};

	struct DirectoryInfo : public RefCounted
	{
		AssetHandle Handle;
		AssetHandle Parent;

		std::string Name;
		std::string FilePath;

		std::vector<AssetHandle> Assets;
		std::vector<AssetHandle> SubDirectories;
	};

	struct SearchResults
	{
		std::vector<Ref<DirectoryInfo>> Directories;
		std::vector<AssetMetadata> Assets;

		void Append(const SearchResults& other)
		{
			Directories.insert(Directories.end(), other.Directories.begin(), other.Directories.end());
			Assets.insert(Assets.end(), other.Assets.begin(), other.Assets.end());
		}
	};

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender();

	private:
		AssetHandle ProcessDirectory(const std::string& directoryPath, AssetHandle parent);

		void DrawDirectoryInfo(AssetHandle directory);

		void RenderDirectory(Ref<DirectoryInfo>& directory);
		void RenderAsset(AssetMetadata& assetInfo);
		void HandleDragDrop(Ref<Image2D> icon, AssetHandle assetHandle);
		void RenderBreadCrumbs();
		void RenderBottomBar();

		void SelectAndStartRenaming(const AssetHandle handle, const std::string& currentName);

		void HandleDirectoryRename(Ref<DirectoryInfo>& dirInfo);
		void HandleAssetRename(AssetMetadata& asset);

		void UpdateCurrentDirectory(AssetHandle directoryHandle);

		void OnFileSystemChanged(FileSystemChangedEvent e);

		void OnAssetDeleted(const FileSystemChangedEvent& e);
		void RemoveDirectory(Ref<DirectoryInfo>& dirInfo);
		void OnDirectoryAdded(const std::string& directoryPath);

		void MoveDirectory(AssetHandle directoryHandle, const std::string& destinationPath);

		Ref<DirectoryInfo> GetDirectoryInfo(const std::string& filepath) const;
		Ref<DirectoryInfo> GetDirectoryForAsset(AssetHandle asset) const;

		SearchResults Search(const std::string& query, AssetHandle directoryHandle);

		template<typename T, typename... Args>
		Ref<T> CreateAsset(const std::string& filename, Args&&... args)
		{
			Ref<Asset> asset = AssetManager::CreateNewAsset<T>(filename, m_CurrentDirectory->FilePath, std::forward<Args>(args)...);

			if (!asset)
				return nullptr;

			m_CurrentDirectory->Assets.push_back(asset->Handle);
			UpdateCurrentDirectory(m_CurrentDirHandle);
			return asset.As<T>();
		}

	private:
		bool m_IsDragging = false;
		bool m_UpdateBreadCrumbs = true;
		bool m_IsAnyItemHovered = false;
		bool m_UpdateDirectoryNextFrame = false;
		bool m_RenamingSelected = false;

		char m_RenameBuffer[MAX_INPUT_BUFFER_LENGTH];
		char m_SearchBuffer[MAX_INPUT_BUFFER_LENGTH];

		AssetHandle m_CurrentDirHandle;
		AssetHandle m_BaseDirectoryHandle;
		AssetHandle m_PrevDirHandle;
		AssetHandle m_NextDirHandle;

		Ref<DirectoryInfo> m_CurrentDirectory;
		Ref<DirectoryInfo> m_BaseDirectory;

		std::vector<Ref<DirectoryInfo>> m_CurrentDirectories;
		std::vector<AssetMetadata> m_CurrentAssets;

		std::vector<Ref<DirectoryInfo>> m_BreadCrumbData;

		SelectionStack m_SelectedAssets;

		std::map<std::string, Ref<Texture2D>> m_AssetIconMap;

		std::unordered_map<AssetHandle, Ref<DirectoryInfo>> m_Directories;
	};

}
