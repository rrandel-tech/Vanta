#pragma once

#include "Asset/AssetManager.hpp"
#include "Renderer/Texture.hpp"
#include "ImGui/ImGui.hpp"

#include <map>

#define MAX_INPUT_BUFFER_LENGTH 128

namespace Vanta {

	template<typename T>
	struct SelectionStack
	{
	public:
		void Select(T item)
		{
			m_Selections.push_back(item);
		}

		void Deselect(T item)
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

		bool IsSelected(T item) const
		{
			for (auto selection : m_Selections)
			{
				if (selection == item)
					return true;
			}

			return false;
		}

		void Clear()
		{
			m_Selections.clear();
		}

		size_t SelectionCount() const
		{
			return m_Selections.size();
		}

		T* GetSelectionData()
		{
			return m_Selections.data();
		}

	private:
		std::vector<T> m_Selections;
	};

	class AssetManagerPanel
	{
	public:
		AssetManagerPanel();

		void OnImGuiRender();

	private:
		void DrawDirectoryInfo(AssetHandle directory);

		void RenderAsset(Ref<Asset>& assetHandle);
		void HandleDragDrop(RendererID icon, Ref<Asset>& asset);
		void RenderBreadCrumbs();

		void HandleRenaming(Ref<Asset>& asset);

		void UpdateCurrentDirectory(AssetHandle directoryHandle);

	private:
		Ref<Texture2D> m_FileTex;
		Ref<Texture2D> m_BackbtnTex;
		Ref<Texture2D> m_FwrdbtnTex;
		Ref<Texture2D> m_FolderRightTex;
		Ref<Texture2D> m_SearchTex;

		std::string m_MovePath;

		bool m_IsDragging = false;
		bool m_UpdateBreadCrumbs = true;
		bool m_IsAnyItemHovered = false;
		bool m_UpdateDirectoryNextFrame = false;

		char m_InputBuffer[MAX_INPUT_BUFFER_LENGTH];

		AssetHandle m_CurrentDirHandle;
		AssetHandle m_BaseDirectoryHandle;
		AssetHandle m_PrevDirHandle;
		AssetHandle m_NextDirHandle;
		Ref<Directory> m_CurrentDirectory;
		Ref<Directory> m_BaseDirectory;
		std::vector<Ref<Asset>> m_CurrentDirAssets;

		std::vector<Ref<Directory>> m_BreadCrumbData;

		AssetHandle m_DraggedAssetId = 0;
		SelectionStack<AssetHandle> m_SelectedAssets;

		bool m_RenamingSelected = false;

		std::map<std::string, Ref<Texture2D>> m_AssetIconMap;
	};

}
