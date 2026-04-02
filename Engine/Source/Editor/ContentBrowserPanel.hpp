#pragma once

#include "Asset/AssetManager.hpp"
#include "Renderer/Texture.hpp"
#include "ImGui/ImGui.hpp"
#include "Project/Project.hpp"

#include "ContentBrowser/ContentBrowserItem.hpp"

#include <map>

#define MAX_INPUT_BUFFER_LENGTH 128

namespace Vanta {

	class SelectionStack
	{
	public:
		void Select(AssetHandle handle)
		{
			if (IsSelected(handle))
				return;

			m_Selections.push_back(handle);
		}

		void Deselect(AssetHandle handle)
		{
			if (!IsSelected(handle))
				return;

			for (auto it = m_Selections.begin(); it != m_Selections.end(); it++)
			{
				if (handle == *it)
				{
					m_Selections.erase(it);
					break;
				}
			}
		}

		bool IsSelected(AssetHandle handle) const
		{
			for (const auto& selectedHandle : m_Selections)
			{
				if (selectedHandle == handle)
					return true;
			}

			return false;
		}

		void Clear()
		{
			m_Selections.clear();
		}

		size_t SelectionCount() const { return m_Selections.size(); }
		const AssetHandle* SelectionData() const { return m_Selections.data(); }

		AssetHandle operator[](size_t index) const
		{
			VA_CORE_ASSERT(index >= 0 && index < m_Selections.size());
			return m_Selections[index];
		}

		std::vector<AssetHandle>::iterator begin() { return m_Selections.begin(); }
		std::vector<AssetHandle>::const_iterator begin() const { return m_Selections.begin(); }
		std::vector<AssetHandle>::iterator end() { return m_Selections.end(); }
		std::vector<AssetHandle>::const_iterator end() const { return m_Selections.end(); }

	private:
		std::vector<AssetHandle> m_Selections;
	};

	struct ContentBrowserItemList
	{
		static constexpr size_t InvalidItem = std::numeric_limits<size_t>::max();

		std::vector<Ref<ContentBrowserItem>> Items;

		std::vector<Ref<ContentBrowserItem>>::iterator begin() { return Items.begin(); }
		std::vector<Ref<ContentBrowserItem>>::iterator end() { return Items.end(); }
		std::vector<Ref<ContentBrowserItem>>::const_iterator begin() const { return Items.begin(); }
		std::vector<Ref<ContentBrowserItem>>::const_iterator end() const { return Items.end(); }

		Ref<ContentBrowserItem>& operator[](size_t index) { return Items[index]; }
		const Ref<ContentBrowserItem>& operator[](size_t index) const { return Items[index]; }

		void erase(AssetHandle handle)
		{
			size_t index = FindItem(handle);
			if (index == InvalidItem)
				return;

			auto it = Items.begin() + index;
			Items.erase(it);
		}

		size_t FindItem(AssetHandle handle) const
		{
			for (size_t i = 0; i < Items.size(); i++)
			{
				if (Items[i]->GetID() == handle)
					return i;
			}

			return InvalidItem;
		}
	};

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel(Ref<Project> project);

		void OnImGuiRender();

		const SelectionStack& GetSelectionStack() const { return m_SelectionStack; }
		ContentBrowserItemList& GetCurrentItems() { return m_CurrentItems; }

		Ref<DirectoryInfo> GetDirectory(const std::string& filepath) const;

	public:
		static ContentBrowserPanel& Get() { return *s_Instance; }

	private:
		AssetHandle ProcessDirectory(const std::string& directoryPath, const Ref<DirectoryInfo>& parent);

		void ChangeDirectory(Ref<DirectoryInfo>& directory);

		void RenderDirectoryHeirarchy(Ref<DirectoryInfo>& directory);
		void RenderTopBar();
		void RenderItems();
		void RenderBottomBar();

		void Refresh();

		void UpdateInput();

		void ClearSelections();

		void RenderDeleteDialogue();
		void RemoveDirectory(Ref<DirectoryInfo>& directory);

		void UpdateDropArea(const Ref<DirectoryInfo>& target);

		void SortItemList();

		ContentBrowserItemList Search(const std::string& query, const Ref<DirectoryInfo>& directoryInfo);

		void OnFileSystemChanged(FileSystemChangedEvent event);

		void OnDirectoryAdded(FileSystemChangedEvent event);
		void OnDirectoryDeleted(FileSystemChangedEvent event);
		void OnDirectoryDeleted(Ref<DirectoryInfo> directory, uint32_t depth = 0);
		void OnDirectoryRenamed(FileSystemChangedEvent event);
		void OnAssetAdded(FileSystemChangedEvent event);
		void OnAssetDeleted(FileSystemChangedEvent event);
		void OnAssetDeleted(AssetMetadata metadata, Ref<DirectoryInfo> directory);
		void OnAssetRenamed(FileSystemChangedEvent event);

		void UpdateDirectoryPath(Ref<DirectoryInfo>& directoryInfo, const std::string& newParentPath);

	private:
		// NOTE: This should only be used within the ContentBrowserPanel!
		//		 For creating a new asset outside the content browser, use AssetManager::CreateNewAsset!
		template<typename T, typename... Args>
		Ref<T> CreateAsset(const std::string& filename, Args&&... args)
		{
			Ref<T> asset = AssetManager::CreateNewAsset<T>(filename, m_CurrentDirectory->FilePath, std::forward<Args>(args)...);
			if (!asset)
				return nullptr;

			m_CurrentDirectory->Assets.push_back(asset->Handle);
			ChangeDirectory(m_CurrentDirectory);

			auto& item = m_CurrentItems[m_CurrentItems.FindItem(asset->Handle)];
			m_SelectionStack.Select(asset->Handle);
			item->SetSelected(true);
			item->StartRenaming();

			return asset;
		}

	private:
		Ref<Project> m_Project;

		Ref<Texture2D> m_FileTex;
		Ref<Texture2D> m_FolderIcon;
		Ref<Texture2D> m_BackbtnTex;
		Ref<Texture2D> m_FwrdbtnTex;
		Ref<Texture2D> m_RefreshIcon;

		std::map<std::string, Ref<Texture2D>> m_AssetIconMap;

		ContentBrowserItemList m_CurrentItems;

		Ref<DirectoryInfo> m_CurrentDirectory;
		Ref<DirectoryInfo> m_BaseDirectory;
		Ref<DirectoryInfo> m_NextDirectory, m_PreviousDirectory;

		bool m_IsAnyItemHovered = false;

		SelectionStack m_SelectionStack;

		std::unordered_map<AssetHandle, Ref<DirectoryInfo>> m_Directories;

		char m_SearchBuffer[MAX_INPUT_BUFFER_LENGTH];

		std::vector<Ref<DirectoryInfo>> m_BreadCrumbData;
		bool m_UpdateNavigationPath = false;

	private:
		static ContentBrowserPanel* s_Instance;
	};

}