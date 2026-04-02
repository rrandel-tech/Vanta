#include "vapch.hpp"
#include "ContentBrowserPanel.hpp"
#include "Core/Application.hpp"
#include "AssetEditorPanel.hpp"
#include "Core/Input.hpp"
#include "Project/Project.hpp"

#include <filesystem>
#include <imgui_internal.h>
#include <stack>

namespace Vanta {

	ContentBrowserPanel::ContentBrowserPanel(Ref<Project> project)
		: m_Project(project)
	{
		s_Instance = this;

		AssetManager::SetAssetChangeCallback(VA_BIND_EVENT_FN(ContentBrowserPanel::OnFileSystemChanged));

		m_FileTex = Texture2D::Create("Resources/Editor/file.png");
		m_FolderIcon = Texture2D::Create("Resources/Editor/folder.png");
		m_AssetIconMap[".fbx"] = Texture2D::Create("Resources/Editor/fbx.png");
		m_AssetIconMap[".obj"] = Texture2D::Create("Resources/Editor/obj.png");
		m_AssetIconMap[".wav"] = Texture2D::Create("Resources/Editor/wav.png");
		m_AssetIconMap[".lua"] = Texture2D::Create("Resources/Editor/lua.png");
		m_AssetIconMap[".png"] = Texture2D::Create("Resources/Editor/png.png");
		m_AssetIconMap[".vscene"] = Texture2D::Create("Resources/Editor/vanta.png");

		m_BackbtnTex = Texture2D::Create("Resources/Editor/btn_back.png");
		m_FwrdbtnTex = Texture2D::Create("Resources/Editor/btn_fwrd.png");
		m_RefreshIcon = Texture2D::Create("Resources/Editor/refresh.png");

		AssetHandle baseDirectoryHandle = ProcessDirectory(project->GetAssetDirectory().string(), nullptr);
		m_BaseDirectory = m_Directories[baseDirectoryHandle];
		ChangeDirectory(m_BaseDirectory);

		memset(m_SearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	AssetHandle ContentBrowserPanel::ProcessDirectory(const std::filesystem::path& directoryPath, const Ref<DirectoryInfo>& parent)
	{
		std::string fixedFilePath = directoryPath.string();
		std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');
		const auto& directory = GetDirectory(fixedFilePath);
		if (directory)
			return directory->Handle;

		Ref<DirectoryInfo> directoryInfo = Ref<DirectoryInfo>::Create();
		directoryInfo->Handle = AssetHandle();
		directoryInfo->Parent = parent;
		directoryInfo->FilePath = fixedFilePath;
		directoryInfo->Name = directoryInfo->FilePath.filename().string();

		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
			{
				AssetHandle subdirHandle = ProcessDirectory(entry.path().string(), directoryInfo);
				directoryInfo->SubDirectories[subdirHandle] = m_Directories[subdirHandle];
				continue;
			}

			auto& metadata = AssetManager::GetMetadata(entry.path().string());
			if (!metadata.IsValid())
			{
				AssetType type = AssetManager::GetAssetTypeFromPath(entry.path());
				if (type == AssetType::None)
					continue;

				metadata.Handle = AssetManager::ImportAsset(entry.path().string());
			}

			// Failed to import
			if (!metadata.IsValid())
				continue;

			directoryInfo->Assets.push_back(metadata.Handle);
		}

		m_Directories[directoryInfo->Handle] = directoryInfo;
		return directoryInfo->Handle;
	}

	void ContentBrowserPanel::ChangeDirectory(Ref<DirectoryInfo>& directory)
	{
		m_UpdateNavigationPath = true;

		m_CurrentItems.Items.clear();

		for (auto&[subdirHandle, subdir] : directory->SubDirectories)
			m_CurrentItems.Items.push_back(Ref<ContentBrowserDirectory>::Create(subdir, m_FolderIcon));

		std::vector<AssetHandle> invalidAssets;
		for (auto assetHandle : directory->Assets)
		{
			AssetMetadata metadata = AssetManager::GetMetadata(assetHandle);

			if (!metadata.IsValid())
				invalidAssets.emplace_back(metadata.Handle);
			else
				m_CurrentItems.Items.push_back(Ref<ContentBrowserAsset>::Create(metadata, m_AssetIconMap.find(metadata.FilePath.extension().string()) != m_AssetIconMap.end() ? m_AssetIconMap[metadata.FilePath.extension().string()] : m_FileTex));
		}

		for (auto invalidHandle : invalidAssets)
			directory->Assets.erase(std::remove(directory->Assets.begin(), directory->Assets.end(), invalidHandle), directory->Assets.end());

		SortItemList();

		m_PreviousDirectory = directory;
		m_CurrentDirectory = directory;
	}

	static int s_ColumnCount = 11;
	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::Begin("Content Browser", NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);

		m_IsContentBrowserHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

		{
			UI::BeginPropertyGrid();

			ImGui::SetColumnOffset(1, 300.0f);
			ImGui::BeginChild("##folders_common");
			{
				if (ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (auto&[handle, directory] : m_BaseDirectory->SubDirectories)
						RenderDirectoryHeirarchy(directory);
				}
			}
			ImGui::EndChild();

			ImGui::NextColumn();

			ImGui::BeginChild("##directory_structure", ImVec2(0, ImGui::GetWindowHeight() - 65));
			{
				RenderTopBar();

				ImGui::Separator();

				ImGui::BeginChild("Scrolling");
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.35f));

					if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
					{
						if (ImGui::BeginMenu("New"))
						{
							if (ImGui::MenuItem("Folder"))
								FileSystem::CreateDirectory((m_CurrentDirectory->FilePath / "New Folder").string());

							ImGui::EndMenu();
						}

						if (ImGui::MenuItem("Import"))
						{
							std::string filepath = Application::Get().OpenFile();
							if (!filepath.empty())
								FileSystem::MoveFile(filepath, m_CurrentDirectory->FilePath.string());
						}

						if (ImGui::MenuItem("Refresh"))
							Refresh();

						ImGui::Separator();

						if (ImGui::MenuItem("Show in Explorer"))
							FileSystem::OpenDirectoryInExplorer(m_CurrentDirectory->FilePath);

						ImGui::EndPopup();
					}

					ImGui::Columns(s_ColumnCount, nullptr, false);

					RenderItems();

					if (ImGui::IsWindowFocused() && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
						UpdateInput();

					ImGui::PopStyleColor(2);

					RenderDeleteDialogue();
				}
				ImGui::EndChild();
			}
			ImGui::EndChild();

			RenderBottomBar();

			UI::EndPropertyGrid();
		}
		ImGui::End();
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectory(const std::filesystem::path& filepath) const
	{
		// TODO: we should store std::filesystem::path here instead of string
		//            so that we don't have to do stuff like this v
		//std::string path = filepath;
		//std::replace(path.begin(), path.end(), '\\', '/');
		for (const auto&[handle, directory] : m_Directories)
		{
			if (directory->FilePath == filepath)
				return directory;
		}

		return nullptr;
	}

	namespace UI {

		static bool TreeNode(const std::string& id, const std::string& label, ImGuiTreeNodeFlags flags = 0)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return false;

			return ImGui::TreeNodeBehavior(window->GetID(id.c_str()), flags, label.c_str(), NULL);
		}
	}

	void ContentBrowserPanel::RenderDirectoryHeirarchy(Ref<DirectoryInfo>& directory)
	{
		std::string id = directory->Name + "_TreeNode";
		bool open = UI::TreeNode(id, directory->Name, ImGuiTreeNodeFlags_SpanAvailWidth);

		UpdateDropArea(directory);

		if (ImGui::IsItemClicked() && directory->Handle != m_CurrentDirectory->Handle)
		{
			if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.01f))
				ChangeDirectory(directory);
		}

		if (open)
		{
			for (auto& [handle, child] : directory->SubDirectories)
				RenderDirectoryHeirarchy(child);

			ImGui::TreePop();
		}
	}

	void ContentBrowserPanel::RenderTopBar()
	{
		ImGui::BeginChild("##top_bar", ImVec2(0, 30));
		{
			if (UI::ImageButton(m_BackbtnTex->GetImage(), ImVec2(25, 25)))
			{
				if (m_CurrentDirectory->Handle == m_BaseDirectory->Handle)
					return;

				m_NextDirectory = m_CurrentDirectory;
				m_PreviousDirectory = m_CurrentDirectory->Parent;
				ChangeDirectory(m_PreviousDirectory);
			}

			ImGui::SameLine();

			if (UI::ImageButton(m_FwrdbtnTex->GetImage(), ImVec2(25, 25)))
				ChangeDirectory(m_NextDirectory);

			ImGui::SameLine();

			if (UI::ImageButton(m_RefreshIcon, ImVec2(25, 25)))
				Refresh();

			ImGui::SameLine();

			{
				ImGui::PushItemWidth(200);
				if (ImGui::InputTextWithHint("##Search", "Search...", m_SearchBuffer, MAX_INPUT_BUFFER_LENGTH))
				{
					if (strlen(m_SearchBuffer) == 0)
					{
						ChangeDirectory(m_CurrentDirectory);
					}
					else
					{
						m_CurrentItems = Search(m_SearchBuffer, m_CurrentDirectory);
						SortItemList();
					}
				}

				ImGui::PopItemWidth();
			}

			ImGui::SameLine();

			if (m_UpdateNavigationPath)
			{
				m_BreadCrumbData.clear();

				Ref<DirectoryInfo> current = m_CurrentDirectory;
				while (current && current->Handle != 0)
				{
					m_BreadCrumbData.push_back(current);
					current = current->Parent;
				}

				std::reverse(m_BreadCrumbData.begin(), m_BreadCrumbData.end());

				m_UpdateNavigationPath = false;
			}

			for (auto& directory : m_BreadCrumbData)
			{
				if (directory->Name != Project::GetActive()->GetConfig().AssetDirectory)
					ImGui::Text("/");

				ImGui::SameLine();

				ImVec2 textSize = ImGui::CalcTextSize(directory->Name.c_str());
				if (ImGui::Selectable(directory->Name.c_str(), false, 0, ImVec2(textSize.x, 22)))
					ChangeDirectory(directory);

				UpdateDropArea(directory);

				ImGui::SameLine();
			}

		}
		ImGui::EndChild();
	}

	static std::mutex s_LockMutex;
	void ContentBrowserPanel::RenderItems()
	{
		m_IsAnyItemHovered = false;

		bool openDeleteDialogue = false;

		std::lock_guard<std::mutex> lock(s_LockMutex);
		for (auto& item : m_CurrentItems)
		{
			item->OnRenderBegin();

			CBItemActionResult result = item->OnRender();

			// There might be a better way of handling this to be honest
			if (result.IsSet(ContentBrowserAction::ClearSelections))
				ClearSelections();

			if (result.IsSet(ContentBrowserAction::Selected) && !m_SelectionStack.IsSelected(item->GetID()))
			{
				m_SelectionStack.Select(item->GetID());
				item->SetSelected(true);
			}

			if (result.IsSet(ContentBrowserAction::DeSelected) && m_SelectionStack.IsSelected(item->GetID()))
			{
				m_SelectionStack.Deselect(item->GetID());
				item->SetSelected(false);
			}

			if (result.IsSet(ContentBrowserAction::SelectToHere) && m_SelectionStack.SelectionCount() == 2)
			{
				size_t firstIndex = m_CurrentItems.FindItem(m_SelectionStack[0]);
				size_t lastIndex = m_CurrentItems.FindItem(item->GetID());

				if (firstIndex > lastIndex)
				{
					size_t temp = firstIndex;
					firstIndex = lastIndex;
					lastIndex = temp;
				}

				for (size_t i = firstIndex; i <= lastIndex; i++)
				{
					auto toSelect = m_CurrentItems[i];
					toSelect->SetSelected(true);
					m_SelectionStack.Select(toSelect->GetID());
				}
			}

			if (result.IsSet(ContentBrowserAction::Reload))
				AssetManager::ReloadData(item->GetID());

			if (result.IsSet(ContentBrowserAction::OpenDeleteDialogue))
				openDeleteDialogue = true;

			if (result.IsSet(ContentBrowserAction::ShowInExplorer))
			{
				if (item->GetType() == ContentBrowserItem::ItemType::Directory)
					FileSystem::ShowFileInExplorer(m_CurrentDirectory->FilePath / item->GetName());
				else
					FileSystem::ShowFileInExplorer(AssetManager::GetFileSystemPath(AssetManager::GetMetadata(item->GetID())));
			}

			if (result.IsSet(ContentBrowserAction::OpenExternal))
			{
				if (item->GetType() == ContentBrowserItem::ItemType::Directory)
					FileSystem::OpenExternally(m_CurrentDirectory->FilePath / item->GetName());
				else
					FileSystem::OpenExternally(AssetManager::GetFileSystemPath(AssetManager::GetMetadata(item->GetID())));
			}

			if (result.IsSet(ContentBrowserAction::Hovered))
				m_IsAnyItemHovered = true;

			item->OnRenderEnd();

			if (result.IsSet(ContentBrowserAction::Renamed))
			{
				SortItemList();
				break;
			}

			if (result.IsSet(ContentBrowserAction::NavigateToThis) && item->GetType() == ContentBrowserItem::ItemType::Directory)
			{
				ChangeDirectory(item.As<ContentBrowserDirectory>()->GetDirectoryInfo());
				break;
			}

			if (result.IsSet(ContentBrowserAction::Refresh))
			{
				ChangeDirectory(m_CurrentDirectory);
				break;
			}
		}

		// This is a workaround an issue with ImGui: https://github.com/ocornut/imgui/issues/331
		if (openDeleteDialogue)
		{
			ImGui::OpenPopup("Delete");
			openDeleteDialogue = false;
		}
	}

	void ContentBrowserPanel::RenderBottomBar()
	{
		ImGui::BeginChild("##panel_controls", ImVec2(ImGui::GetColumnWidth() - 12, 30), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			ImGui::Separator();

			ImGui::Columns(4, 0, false);

			if (m_SelectionStack.SelectionCount() == 1)
			{
				const AssetMetadata& asset = AssetManager::GetMetadata(m_SelectionStack[0]);

				std::string filepath;
				if (asset.IsValid())
				{
					filepath = asset.FilePath.string();
				}
				else if (m_Directories.find(m_SelectionStack[0]) != m_Directories.end())
				{
					filepath = std::filesystem::relative(m_Directories[m_SelectionStack[0]]->FilePath, Project::GetAssetDirectory()).string();
					std::replace(filepath.begin(), filepath.end(), '\\', '/');
				}

				ImGui::Text(filepath.c_str());
			}
			else if (m_SelectionStack.SelectionCount() > 1)
			{
				ImGui::Text("%d items selected", m_SelectionStack.SelectionCount());
			}

			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
			ImGui::SliderInt("##column_count", &s_ColumnCount, 2, 15);
		}
		ImGui::EndChild();
	}


	void ContentBrowserPanel::Refresh()
	{
		for (auto entry : std::filesystem::directory_iterator(m_CurrentDirectory->FilePath))
		{
			if (!entry.is_directory())
			{
				const auto& assetInfo = AssetManager::GetMetadata(entry.path().string());
				if (!assetInfo.IsValid())
				{
					AssetHandle handle = AssetManager::ImportAsset(entry.path().string());
					m_CurrentDirectory->Assets.push_back(handle);
				}
			}
			else
			{
				std::string fixedFilePath = entry.path().string();
				std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');

				const auto& directory = GetDirectory(fixedFilePath);
				if (!directory)
				{
					AssetHandle directoryHandle = ProcessDirectory(entry.path().string(), m_CurrentDirectory);
					m_CurrentDirectory->SubDirectories[directoryHandle] = m_Directories[directoryHandle];
				}
			}
		}

		ChangeDirectory(m_CurrentDirectory);
	}

	void ContentBrowserPanel::UpdateInput()
	{
		if (!m_IsContentBrowserHovered)
			return;

		if ((!m_IsAnyItemHovered && ImGui::IsAnyMouseDown()) || Input::IsKeyPressed(KeyCode::Escape))
			ClearSelections();

		if (Input::IsKeyPressed(KeyCode::Delete))
			ImGui::OpenPopup("Delete");

		if (Input::IsKeyPressed(KeyCode::F5))
			Refresh();
	}

	void ContentBrowserPanel::ClearSelections()
	{
		for (auto selectedHandle : m_SelectionStack)
		{
			for (auto& item : m_CurrentItems)
			{
				if (item->GetID() == selectedHandle)
					item->SetSelected(false);
			}
		}

		m_SelectionStack.Clear();
	}

	void ContentBrowserPanel::RenderDeleteDialogue()
	{
		if (ImGui::BeginPopupModal("Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to delete %d items?", m_SelectionStack.SelectionCount());

			float columnWidth = ImGui::GetContentRegionAvail().x / 4;

			ImGui::Columns(4, 0, false);
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::SetColumnWidth(1, columnWidth);
			ImGui::SetColumnWidth(2, columnWidth);
			ImGui::SetColumnWidth(3, columnWidth);
			ImGui::NextColumn();
			if (ImGui::Button("Yes", ImVec2(columnWidth, 0)))
			{
				for (AssetHandle handle : m_SelectionStack)
				{
					size_t index = m_CurrentItems.FindItem(handle);
					if (index == ContentBrowserItemList::InvalidItem)
						continue;

					m_CurrentItems[index]->Delete();
					m_CurrentItems.erase(handle);
				}

				for (AssetHandle handle : m_SelectionStack)
				{
					if (m_Directories.find(handle) != m_Directories.end())
						RemoveDirectory(m_Directories[handle]);
				}

				m_SelectionStack.Clear();

				ChangeDirectory(m_CurrentDirectory);

				ImGui::CloseCurrentPopup();
			}

			ImGui::NextColumn();
			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("No", ImVec2(columnWidth, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::NextColumn();
			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::RemoveDirectory(Ref<DirectoryInfo>& directory)
	{
		if (directory->Parent)
		{
			auto& childList = directory->Parent->SubDirectories;
			childList.erase(childList.find(directory->Handle));
		}

		for (auto&[handle, subdir] : directory->SubDirectories)
			RemoveDirectory(subdir);

		directory->SubDirectories.clear();
		directory->Assets.clear();

		m_Directories.erase(m_Directories.find(directory->Handle));
	}

	void ContentBrowserPanel::UpdateDropArea(const Ref<DirectoryInfo>& target)
	{
		if (target->Handle != m_CurrentDirectory->Handle && ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("asset_payload");

			if (payload)
			{
				uint32_t count = payload->DataSize / sizeof(AssetHandle);

				for (uint32_t i = 0; i < count; i++)
				{
					AssetHandle assetHandle = *(((AssetHandle*)payload->Data) + i);
					size_t index = m_CurrentItems.FindItem(assetHandle);
					if (index != ContentBrowserItemList::InvalidItem)
					{
						m_CurrentItems[index]->Move(target->FilePath);
						m_CurrentItems.erase(assetHandle);
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	}

	void ContentBrowserPanel::SortItemList()
	{
		std::sort(m_CurrentItems.begin(), m_CurrentItems.end(), [](const Ref<ContentBrowserItem>& item1, const Ref<ContentBrowserItem>& item2)
		{
			if (item1->GetType() == item2->GetType())
				return Utils::ToLower(item1->GetName()) < Utils::ToLower(item2->GetName());

			return (uint16_t)item1->GetType() < (uint16_t)item2->GetType();
		});
	}

	ContentBrowserItemList ContentBrowserPanel::Search(const std::string& query, const Ref<DirectoryInfo>& directoryInfo)
	{
		ContentBrowserItemList results;
		std::string queryLowerCase = Utils::ToLower(query);

		for (auto&[handle, subdir] : directoryInfo->SubDirectories)
		{
			if (subdir->Name.find(queryLowerCase) != std::string::npos)
				results.Items.push_back(Ref<ContentBrowserDirectory>::Create(subdir, m_FolderIcon));

			ContentBrowserItemList list = Search(query, subdir);
			results.Items.insert(results.Items.end(), list.Items.begin(), list.Items.end());
		}

		for (auto& assetHandle : directoryInfo->Assets)
		{
			auto& asset = AssetManager::GetMetadata(assetHandle);
			std::string filename = Utils::ToLower(asset.FilePath.filename().string());

			if (filename.find(queryLowerCase) != std::string::npos)
				results.Items.push_back(Ref<ContentBrowserAsset>::Create(asset, m_AssetIconMap.find(asset.FilePath.extension().string()) != m_AssetIconMap.end() ? m_AssetIconMap[asset.FilePath.extension().string()] : m_FileTex));

			if (queryLowerCase[0] != '.')
				continue;

			if (asset.FilePath.extension().string().find(std::string(&queryLowerCase[1])) != std::string::npos)
				results.Items.push_back(Ref<ContentBrowserAsset>::Create(asset, m_AssetIconMap.find(asset.FilePath.extension().string()) != m_AssetIconMap.end() ? m_AssetIconMap[asset.FilePath.extension().string()] : m_FileTex));
		}

		return results;
	}

	void ContentBrowserPanel::OnFileSystemChanged(FileSystemChangedEvent event)
	{
		std::lock_guard<std::mutex> lock(s_LockMutex);
		switch (event.Action)
		{
		case FileSystemAction::Added:
		{
			if (event.IsDirectory)
				OnDirectoryAdded(event);
			else
				OnAssetAdded(event);
			break;
		}
		case FileSystemAction::Delete:
		{
			if (event.IsDirectory)
				OnDirectoryDeleted(event);
			else
				OnAssetDeleted(event);
			break;
		}
		case FileSystemAction::Modified:
		{
			// It doesn't make sense to handle this for directories, and asset modification should be handle by the AssetManager
			break;
		}
		case FileSystemAction::Rename:
		{
			if (event.IsDirectory)
				OnDirectoryRenamed(event);
			else
				OnAssetRenamed(event);
			break;
		}
		}
	}

	void ContentBrowserPanel::OnAssetAdded(FileSystemChangedEvent event)
	{
		const auto& assetMetadata = AssetManager::GetMetadata(event.FilePath.string());
		if (!assetMetadata.IsValid())
			return;

		auto directory = GetDirectory(event.FilePath.parent_path());
		if (!directory)
			VA_CORE_ASSERT(false, "How did this even happen?");

		directory->Assets.push_back(assetMetadata.Handle);

		ChangeDirectory(m_CurrentDirectory);
	}

	void ContentBrowserPanel::OnDirectoryAdded(FileSystemChangedEvent event)
	{
		// FileSystemChangedEvents are relative to asset directory, but some
		// ContentBrowserPanel functions are relative to working directory
		std::filesystem::path directoryPath = Project::GetActive()->GetAssetDirectory() / event.FilePath;
		auto parentDirectory = GetDirectory(directoryPath.parent_path().string());
		if (!parentDirectory)
			VA_CORE_ASSERT(false, "How did this even happen?");

		AssetHandle directoryHandle = ProcessDirectory(directoryPath.string(), parentDirectory);
		if (directoryHandle == 0)
			return;

		parentDirectory->SubDirectories[directoryHandle] = m_Directories[directoryHandle];

		ChangeDirectory(m_CurrentDirectory);

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
		{
			size_t directoryIndex = m_CurrentItems.FindItem(directoryHandle);
			if (directoryIndex == ContentBrowserItemList::InvalidItem)
				return;

			auto& item = m_CurrentItems[directoryIndex];
			m_SelectionStack.Select(directoryHandle);
			item->SetSelected(true);
			item->StartRenaming();
		}
	}

	void ContentBrowserPanel::OnAssetDeleted(FileSystemChangedEvent event)
	{
		std::filesystem::path directoryPath = Project::GetActive()->GetAssetDirectory() / event.FilePath;
		AssetMetadata metadata;
		Ref<DirectoryInfo> directory;
		for (const auto& item : m_CurrentItems.Items)
		{
			if (item->GetType() == ContentBrowserItem::ItemType::Asset)
			{
				const auto& assetInfo = item.As<ContentBrowserAsset>()->GetAssetInfo();

				if (assetInfo.FilePath == event.FilePath)
				{
					metadata = assetInfo;

					std::string parentDirectory = directoryPath.parent_path().string();
					directory = GetDirectory(parentDirectory);
					break;
				}
			}
		}

		OnAssetDeleted(metadata, directory);
		ChangeDirectory(m_CurrentDirectory);
	}

	void ContentBrowserPanel::OnAssetDeleted(AssetMetadata metadata, Ref<DirectoryInfo> directory)
	{
		if (!metadata.IsValid() || !directory)
			return;

		if (AssetManager::IsAssetHandleValid(metadata.Handle))
			AssetManager::OnAssetDeleted(metadata.Handle);

		directory->Assets.erase(std::remove(directory->Assets.begin(), directory->Assets.end(), metadata.Handle), directory->Assets.end());
		m_CurrentItems.erase(metadata.Handle);
	}

	void ContentBrowserPanel::OnAssetRenamed(FileSystemChangedEvent event)
	{
		if (!event.WasTracking)
		{
			auto& assetInfo = AssetManager::GetMetadata(event.FilePath.string());
			auto directory = GetDirectory(event.FilePath.parent_path());
			directory->Assets.push_back(assetInfo.Handle);
			ChangeDirectory(m_CurrentDirectory);
		}
		else
		{
			auto& assetInfo = AssetManager::GetMetadata(event.FilePath.string());
			if (!assetInfo.IsValid())
				return;

			size_t index = m_CurrentItems.FindItem(assetInfo.Handle);
			if (index != ContentBrowserItemList::InvalidItem)
				m_CurrentItems[index]->Rename(event.NewName, true);
		}
	}

	void ContentBrowserPanel::OnDirectoryDeleted(FileSystemChangedEvent event)
	{
		OnDirectoryDeleted(GetDirectory(event.FilePath));
	}

	void ContentBrowserPanel::OnDirectoryDeleted(Ref<DirectoryInfo> directory, uint32_t depth)
	{
		if (!directory)
			return;

		for (auto asset : directory->Assets)
			OnAssetDeleted(AssetManager::GetMetadata(asset), directory);

		for (auto[subdirHandle, subdir] : directory->SubDirectories)
			OnDirectoryDeleted(subdir, depth + 1);

		directory->Assets.clear();
		directory->SubDirectories.clear();

		if (depth == 0 && directory->Parent)
			directory->Parent->SubDirectories.erase(directory->Handle);

		m_CurrentItems.erase(directory->Handle);
		m_Directories.erase(directory->Handle);

		ChangeDirectory(m_CurrentDirectory);
	}

	void ContentBrowserPanel::OnDirectoryRenamed(FileSystemChangedEvent event)
	{
		auto directory = GetDirectory(event.FilePath.parent_path() / event.OldName);

		size_t itemIndex = m_CurrentItems.FindItem(directory->Handle);
		if (itemIndex != ContentBrowserItemList::InvalidItem)
		{
			m_CurrentItems[itemIndex]->Rename(event.NewName, true);
		}
		else
		{
			directory->Name = event.NewName;
			UpdateDirectoryPath(directory, event.FilePath.parent_path().string());
		}

		ChangeDirectory(m_CurrentDirectory);
	}

	void ContentBrowserPanel::UpdateDirectoryPath(Ref<DirectoryInfo>& directoryInfo, const std::filesystem::path& newParentPath)
	{
		directoryInfo->FilePath = newParentPath / directoryInfo->Name;

		for (auto& assetHandle : directoryInfo->Assets)
		{
			auto& metadata = AssetManager::GetMetadata(assetHandle);
			metadata.FilePath = directoryInfo->FilePath / metadata.FilePath.filename();
		}

		for (auto& [handle, subdirectory] : directoryInfo->SubDirectories)
			UpdateDirectoryPath(subdirectory, directoryInfo->FilePath);
	}

	ContentBrowserPanel* ContentBrowserPanel::s_Instance;

}