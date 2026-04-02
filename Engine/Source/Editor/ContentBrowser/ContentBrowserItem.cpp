#include "vapch.hpp"
#include "ContentBrowserItem.hpp"
#include "Asset/AssetManager.hpp"
#include "Utilities/FileSystem.hpp"
#include "ImGui/ImGui.hpp"
#include "Editor/ContentBrowserPanel.hpp"
#include "Editor/AssetEditorPanel.hpp"

#include "imgui_internal.h"

#include <filesystem>

namespace Vanta {

	static char s_RenameBuffer[MAX_INPUT_BUFFER_LENGTH];

	ContentBrowserItem::ContentBrowserItem(ItemType type, AssetHandle id, const std::string& name, const Ref<Texture2D>& icon)
		: m_Type(type), m_ID(id), m_Name(name), m_Icon(icon)
	{
	}

	void ContentBrowserItem::OnRenderBegin()
	{
		ImGui::PushID(&m_ID);
		ImGui::BeginGroup();
	}

	CBItemActionResult ContentBrowserItem::OnRender()
	{
		CBItemActionResult result;

		if (m_IsSelected)
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));

		float buttonWidth = ImGui::GetColumnWidth() - 15.0f;
		UI::ImageButton(m_Name.c_str(), m_Icon, { buttonWidth, buttonWidth });

		if (m_IsSelected)
			ImGui::PopStyleColor();

		UpdateDrop(result);

		bool dragging = false;
		if (dragging = ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			m_IsDragging = true;

			const auto& selectionStack = ContentBrowserPanel::Get().GetSelectionStack();
			if (!selectionStack.IsSelected(m_ID))
				result.Set(ContentBrowserAction::ClearSelections, true);


			auto& currentItems = ContentBrowserPanel::Get().GetCurrentItems();

			if (selectionStack.SelectionCount() > 0)
			{
				for (const auto& selectedItemHandles : selectionStack)
				{
					size_t index = currentItems.FindItem(selectedItemHandles);
					if (index == ContentBrowserItemList::InvalidItem)
						continue;

					const auto& item = currentItems[index];
					UI::Image(item->GetIcon(), ImVec2(20, 20));
					ImGui::SameLine();
					const auto& name = item->GetName();
					ImGui::TextUnformatted(name.c_str());
				}

				ImGui::SetDragDropPayload("asset_payload", selectionStack.SelectionData(), sizeof(AssetHandle) * selectionStack.SelectionCount());
			}

			result.Set(ContentBrowserAction::Selected, true);
			ImGui::EndDragDropSource();
		}

		if (ImGui::IsItemHovered())
		{
			result.Set(ContentBrowserAction::Hovered, true);

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				Activate(result);
			}
			else
			{
				const auto& selectionStack = ContentBrowserPanel::Get().GetSelectionStack();

				bool action = selectionStack.SelectionCount() > 1 ? ImGui::IsMouseReleased(ImGuiMouseButton_Left) : ImGui::IsMouseClicked(ImGuiMouseButton_Left);
				bool skipBecauseDragging = m_IsDragging && selectionStack.IsSelected(m_ID);
				if (action && !skipBecauseDragging)
				{
					result.Set(ContentBrowserAction::Selected, true);

					if (!Input::IsKeyPressed(KeyCode::LeftControl) && !Input::IsKeyPressed(KeyCode::LeftShift))
						result.Set(ContentBrowserAction::ClearSelections, true);

					if (Input::IsKeyPressed(KeyCode::LeftShift))
						result.Set(ContentBrowserAction::SelectToHere, true);
				}
			}

		}

		if (ImGui::BeginPopupContextItem("CBItemContextMenu"))
		{
			result.Set(ContentBrowserAction::Selected, true);
			OnContextMenuOpen(result);
			ImGui::EndPopup();
		}

		if (!m_IsRenaming)
		{
			ImGui::TextWrapped(m_Name.c_str());

			if (Input::IsKeyPressed(KeyCode::F2) && m_IsSelected)
				StartRenaming();
		}
		else
		{
			ImGui::SetKeyboardFocusHere();
			if (ImGui::InputText("##rename", s_RenameBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				Rename(s_RenameBuffer);
				m_IsRenaming = false;
				result.Set(ContentBrowserAction::Renamed, true);
			}
		}

		m_IsDragging = dragging;

		return result;
	}

	void ContentBrowserItem::StartRenaming()
	{
		if (m_IsRenaming)
			return;

		memset(s_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
		memcpy(s_RenameBuffer, m_Name.c_str(), m_Name.size());
		m_IsRenaming = true;
	}

	void ContentBrowserItem::SetSelected(bool value)
	{
		m_IsSelected = value;

		if (!m_IsSelected)
		{
			m_IsRenaming = false;
			memset(s_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
		}
	}

	void ContentBrowserItem::Rename(const std::string& newName, bool fromCallback)
	{
		OnRenamed(newName, fromCallback);
		m_Name = newName;
	}

	void ContentBrowserItem::OnContextMenuOpen(CBItemActionResult& actionResult)
	{
		if (ImGui::MenuItem("Reload"))
			actionResult.Set(ContentBrowserAction::Reload, true);

		if (ImGui::MenuItem("Rename"))
			StartRenaming();

		if (ImGui::MenuItem("Delete"))
			actionResult.Set(ContentBrowserAction::OpenDeleteDialogue, true);

		ImGui::Separator();

		if (ImGui::MenuItem("Show In Explorer"))
			actionResult.Set(ContentBrowserAction::ShowInExplorer, true);
		if (ImGui::MenuItem("Open Externally"))
			actionResult.Set(ContentBrowserAction::OpenExternal, true);

		RenderCustomContextItems();
	}

	void ContentBrowserItem::OnRenderEnd()
	{
		ImGui::EndGroup();
		ImGui::PopID();
		ImGui::NextColumn();
	}

	ContentBrowserDirectory::ContentBrowserDirectory(const Ref<DirectoryInfo>& directoryInfo, const Ref<Texture2D>& icon)
		: ContentBrowserItem(ContentBrowserItem::ItemType::Directory, directoryInfo->Handle, directoryInfo->Name, icon), m_DirectoryInfo(directoryInfo)
	{
	}

	ContentBrowserDirectory::~ContentBrowserDirectory()
	{
	}
	
	void ContentBrowserDirectory::Activate(CBItemActionResult& actionResult)
	{
		actionResult.Set(ContentBrowserAction::NavigateToThis, true);
	}

	void ContentBrowserDirectory::OnRenamed(const std::string& newName, bool fromCallback)
	{
		if (!fromCallback)
		{
			if (FileSystem::Exists((m_DirectoryInfo->FilePath / newName).string()))
			{
				VA_CORE_ERROR("A directory with that name already exists!");
				return;
			}

			FileSystem::Rename(m_DirectoryInfo->FilePath.string(), newName);
		}

		m_DirectoryInfo->Name = newName;
		UpdateDirectoryPath(m_DirectoryInfo, m_DirectoryInfo->FilePath.parent_path().string());
	}

	void ContentBrowserDirectory::UpdateDrop(CBItemActionResult& actionResult)
	{
		if (IsSelected())
			return;

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("asset_payload");

			if (payload)
			{
				auto& currentItems = ContentBrowserPanel::Get().GetCurrentItems();
				uint32_t count = payload->DataSize / sizeof(AssetHandle);

				for (uint32_t i = 0; i < count; i++)
				{
					AssetHandle assetHandle = *(((AssetHandle*)payload->Data) + i);
					size_t index = currentItems.FindItem(assetHandle);
					if (index != ContentBrowserItemList::InvalidItem)
					{
						if (currentItems[index]->Move(m_DirectoryInfo->FilePath))
						{
							actionResult.Set(ContentBrowserAction::Refresh, true);
							currentItems.erase(assetHandle);
						}
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	}

	void ContentBrowserDirectory::Delete()
	{
		FileSystem::DeleteFile(m_DirectoryInfo->FilePath.string());
	}

	bool ContentBrowserDirectory::Move(const std::filesystem::path& destination)
	{
		bool wasMoved = FileSystem::MoveFile(m_DirectoryInfo->FilePath.string(), destination.string());
		if (!wasMoved)
			return false;

		UpdateDirectoryPath(m_DirectoryInfo, destination);

		auto& parentSubdirs = m_DirectoryInfo->Parent->SubDirectories;
		parentSubdirs.erase(parentSubdirs.find(m_DirectoryInfo->Handle));

		auto newParent = ContentBrowserPanel::Get().GetDirectory(destination);
		newParent->SubDirectories[m_DirectoryInfo->Handle] = m_DirectoryInfo;
		m_DirectoryInfo->Parent = newParent;

		return true;
	}

	void ContentBrowserDirectory::UpdateDirectoryPath(Ref<DirectoryInfo> directoryInfo, const std::filesystem::path& newParentPath)
	{
		directoryInfo->FilePath = newParentPath / directoryInfo->Name;

		for (auto assetHandle : directoryInfo->Assets)
		{
			auto metadata = AssetManager::GetMetadata(assetHandle);
			metadata.FilePath = directoryInfo->FilePath / metadata.FilePath.filename();
		}

		for (auto[handle, subdirectory] : directoryInfo->SubDirectories)
			UpdateDirectoryPath(subdirectory, directoryInfo->FilePath);
	}

	ContentBrowserAsset::ContentBrowserAsset(const AssetMetadata& assetInfo, const Ref<Texture2D>& icon)
		: ContentBrowserItem(ContentBrowserItem::ItemType::Asset, assetInfo.Handle, assetInfo.FilePath.stem().string(), icon), m_AssetInfo(assetInfo)
	{
	}

	ContentBrowserAsset::~ContentBrowserAsset()
	{

	}

	void ContentBrowserAsset::Delete()
	{
		bool deleted = FileSystem::DeleteFile(m_AssetInfo.FilePath.string());
		if (!deleted)
		{
			VA_CORE_ERROR("Couldn't delete {0}", m_AssetInfo.FilePath.string());
			return;
		}

		auto currentDirectory = ContentBrowserPanel::Get().GetDirectory(m_AssetInfo.FilePath.parent_path().string());
		currentDirectory->Assets.erase(std::remove(currentDirectory->Assets.begin(), currentDirectory->Assets.end(), m_AssetInfo.Handle), currentDirectory->Assets.end());

		AssetManager::OnAssetDeleted(m_AssetInfo.Handle);
	}

	bool ContentBrowserAsset::Move(const std::filesystem::path& destination)
	{
		bool wasMoved = FileSystem::MoveFile(m_AssetInfo.FilePath.string(), destination.string());
		if (!wasMoved)
		{
			VA_CORE_ERROR("Couldn't move {0} to {1}", m_AssetInfo.FilePath.string(), destination.string());
			return false;
		}

		const auto& currentDirectory = ContentBrowserPanel::Get().GetDirectory(m_AssetInfo.FilePath.parent_path().string());
		// currentDirectory->Assets.erase(std::remove(currentDirectory->Assets.begin(), currentDirectory->Assets.end(), m_AssetInfo.Handle), currentDirectory->Assets.end());

		AssetManager::OnAssetMoved(m_AssetInfo.Handle, destination.string());

		return true;
	}

	void ContentBrowserAsset::Activate(CBItemActionResult& actionResult)
	{
		if (m_AssetInfo.Type == AssetType::Scene)
		{
			// TODO: Open in Viewport
		}
		else
		{
			AssetEditorPanel::OpenEditor(AssetManager::GetAsset<Asset>(m_AssetInfo.Handle));
		}
	}

	void ContentBrowserAsset::OnRenamed(const std::string& newName, bool fromCallback)
	{
		std::string newFilePath = std::format("{0}/{1}.{2}", m_AssetInfo.FilePath.parent_path().string(), '.', m_AssetInfo.FilePath.extension().string());

		if (!fromCallback)
		{
			if (FileSystem::Exists(newFilePath))
			{
				VA_CORE_ERROR("A file with that name already exists!");
				return;
			}

			FileSystem::Rename(m_AssetInfo.FilePath.string(), newName);
			AssetManager::OnAssetRenamed(m_AssetInfo.Handle, newFilePath);
		}

		m_AssetInfo.FilePath = AssetManager::GetRelativePath(newFilePath);
	}

}