#include "vapch.hpp"
#include "AssetManagerPanel.hpp"
#include "Core/Application.hpp"
#include "AssetEditorPanel.hpp"
#include "Core/Input.hpp"

namespace Vanta {

	AssetManagerPanel::AssetManagerPanel()
	{
		AssetManager::SetAssetChangeCallback([&]()
		{
			UpdateCurrentDirectory(m_CurrentDirHandle);
		});

		m_FileTex = AssetManager::GetAsset<Texture2D>("assets/editor/file.png");
		m_AssetIconMap[""] = AssetManager::GetAsset<Texture2D>("assets/editor/folder.png");
		m_AssetIconMap["fbx"] = AssetManager::GetAsset<Texture2D>("assets/editor/fbx.png");
		m_AssetIconMap["obj"] = AssetManager::GetAsset<Texture2D>("assets/editor/obj.png");
		m_AssetIconMap["wav"] = AssetManager::GetAsset<Texture2D>("assets/editor/wav.png");
		m_AssetIconMap["lua"] = AssetManager::GetAsset<Texture2D>("assets/editor/lua.png");
		m_AssetIconMap["png"] = AssetManager::GetAsset<Texture2D>("assets/editor/png.png");
		m_AssetIconMap["blend"] = AssetManager::GetAsset<Texture2D>("assets/editor/blend.png");
		m_AssetIconMap["hsc"] = AssetManager::GetAsset<Texture2D>("assets/editor/vanta.png");

		m_BackbtnTex = AssetManager::GetAsset<Texture2D>("assets/editor/btn_back.png");
		m_FwrdbtnTex = AssetManager::GetAsset<Texture2D>("assets/editor/btn_fwrd.png");
		m_FolderRightTex = AssetManager::GetAsset<Texture2D>("assets/editor/folder_hierarchy.png");
		m_SearchTex = AssetManager::GetAsset<Texture2D>("assets/editor/search.png");

		m_BaseDirectoryHandle = AssetManager::GetAssetHandleFromFilePath("assets");
		m_BaseDirectory = AssetManager::GetAsset<Directory>(m_BaseDirectoryHandle);
		UpdateCurrentDirectory(m_BaseDirectoryHandle);

		memset(m_InputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	void AssetManagerPanel::DrawDirectoryInfo(AssetHandle directory)
	{
		const Ref<Directory>& dir = AssetManager::GetAsset<Directory>(directory);

		if (ImGui::TreeNode(dir->FileName.c_str()))
		{
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				UpdateCurrentDirectory(directory);

			for (AssetHandle child : dir->ChildDirectories)
			{
				DrawDirectoryInfo(child);
			}

			ImGui::TreePop();
		}
	}

	static int s_ColumnCount = 10;
	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin("Project", NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
		{
			UI::BeginPropertyGrid();
			ImGui::SetColumnOffset(1, 240);

			ImGui::BeginChild("##folders_common");
			{
				if (ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (AssetHandle child : m_BaseDirectory->ChildDirectories)
					{
						DrawDirectoryInfo(child);
					}
				}
			}
			ImGui::EndChild();

			ImGui::NextColumn();

			ImGui::BeginChild("##directory_structure", ImVec2(ImGui::GetColumnWidth() - 12, ImGui::GetWindowHeight() - 60));
			{
				ImGui::BeginChild("##directory_breadcrumbs", ImVec2(ImGui::GetColumnWidth() - 100, 30));
				RenderBreadCrumbs();
				ImGui::EndChild();

				ImGui::BeginChild("Scrolling");

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.205f, 0.21f, 0.25f));

				if (Input::IsKeyPressed(KeyCode::Escape) || (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_IsAnyItemHovered))
				{
					m_SelectedAssets.Clear();
					m_RenamingSelected = false;
					memset(m_InputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
				}

				m_IsAnyItemHovered = false;

				if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
				{
					if (ImGui::BeginMenu("Create"))
					{
						if (ImGui::MenuItem("Folder"))
						{
							bool created = FileSystem::CreateFolder(m_CurrentDirectory->FilePath + "/New Folder");

							if (created)
							{
								UpdateCurrentDirectory(m_CurrentDirHandle);
								auto createdDirectory = AssetManager::GetAsset<Directory>(AssetManager::GetAssetHandleFromFilePath(m_CurrentDirectory->FilePath + "/New Folder"));
								m_SelectedAssets.Select(createdDirectory->Handle);
								memset(m_InputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
								memcpy(m_InputBuffer, createdDirectory->FileName.c_str(), createdDirectory->FileName.size());
								m_RenamingSelected = true;
							}
						}

						if (ImGui::MenuItem("Scene"))
						{
							VA_CORE_INFO("Creating Scene...");
						}

						if (ImGui::MenuItem("Script"))
						{
							VA_CORE_INFO("Creating Script...");
						}

						if (ImGui::MenuItem("Prefab"))
						{
							VA_CORE_INFO("Creating Prefab...");
						}

						ImGui::EndMenu();
					}

					if (ImGui::MenuItem("Import"))
					{
					}

					if (ImGui::MenuItem("Refresh"))
					{
						UpdateCurrentDirectory(m_CurrentDirHandle);
					}

					ImGui::EndPopup();
				}

				ImGui::Columns(s_ColumnCount, nullptr, false);
				
				for (Ref<Asset>& asset : m_CurrentDirAssets)
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
					m_DraggedAssetId = 0;
				}

				ImGui::PopStyleColor(2);

				ImGui::EndChild();
			}
			ImGui::EndChild();

			ImGui::BeginChild("##panel_controls", ImVec2(ImGui::GetColumnWidth() - 12, 20), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			ImGui::Columns(4, 0, false);
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
			ImGui::SliderInt("##column_count", &s_ColumnCount, 2, 15);
			ImGui::EndChild();

			UI::EndPropertyGrid();
		}
		ImGui::End();
	}

	void AssetManagerPanel::RenderAsset(Ref<Asset>& asset)
	{
		// These caches are currently required for when we change directories
		AssetHandle assetHandle = asset->Handle;
		std::string filename = asset->FileName;

		ImGui::PushID(&asset->Handle);
		ImGui::BeginGroup();

		RendererID iconRef = m_AssetIconMap.find(asset->Extension) != m_AssetIconMap.end() ? m_AssetIconMap[asset->Extension]->GetRendererID() : m_FileTex->GetRendererID();

		if (m_SelectedAssets.IsSelected(assetHandle))
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));

		float buttonWidth = ImGui::GetColumnWidth() - 15.0f;
		ImGui::ImageButton("##icon", (ImTextureID)iconRef, { buttonWidth, buttonWidth });

		if (m_SelectedAssets.IsSelected(assetHandle))
			ImGui::PopStyleColor();

		HandleDragDrop(iconRef, asset);

		if (ImGui::IsItemHovered())
		{
			m_IsAnyItemHovered = true;

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (asset->Type == AssetType::Directory)
				{
					m_PrevDirHandle = m_CurrentDirHandle;
					m_CurrentDirHandle = assetHandle;
					m_UpdateDirectoryNextFrame = true;
				}
				else if (asset->Type == AssetType::Scene)
				{
					// SceneManager::OpenScene(asset);
				}
				else
				{
					AssetEditorPanel::OpenEditor(asset);
				}
			}

			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_IsDragging)
			{
				if (!Input::IsKeyPressed(KeyCode::LeftControl))
					m_SelectedAssets.Clear();

				if (m_SelectedAssets.IsSelected(assetHandle))
					m_SelectedAssets.Deselect(assetHandle);
				else
					m_SelectedAssets.Select(assetHandle);
			}
		}

		bool shouldOpenDeleteModal = false;

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Rename"))
			{
				m_SelectedAssets.Select(assetHandle);
				memset(m_InputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
				memcpy(m_InputBuffer, filename.c_str(), filename.size());
				m_RenamingSelected = true;
			}

			if (ImGui::MenuItem("Delete"))
				shouldOpenDeleteModal = true;

			ImGui::EndPopup();
		}

		if (shouldOpenDeleteModal)
		{
			ImGui::OpenPopup("Delete Asset");
			shouldOpenDeleteModal = false;
		}

		bool deleted = false;
		if (ImGui::BeginPopupModal("Delete Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (asset->Type == AssetType::Directory)
				ImGui::Text("Are you sure you want to delete %s and everything within it?", filename.c_str());
			else
				ImGui::Text("Are you sure you want to delete %s?", filename.c_str());

			float columnWidth = ImGui::GetContentRegionAvail().x / 4;

			ImGui::Columns(4, 0, false);
			ImGui::SetColumnWidth(0, columnWidth);
			ImGui::SetColumnWidth(1, columnWidth);
			ImGui::SetColumnWidth(2, columnWidth);
			ImGui::SetColumnWidth(3, columnWidth);
			ImGui::NextColumn();
			if (ImGui::Button("Yes", ImVec2(columnWidth, 0)))
			{
				// Cache this so that we can delete the meta file if the asset was deleted successfully
				std::string filepath = asset->FilePath;
				deleted = FileSystem::DeleteFile(filepath);
				if (deleted)
				{
					FileSystem::DeleteFile(filepath + ".meta");
					AssetManager::RemoveAsset(assetHandle);
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

			if (!m_SelectedAssets.IsSelected(assetHandle) || !m_RenamingSelected)
				ImGui::TextWrapped(filename.c_str());

			if (m_SelectedAssets.IsSelected(assetHandle))
				HandleRenaming(asset);
		}

		ImGui::EndGroup();
		ImGui::PopID();
	}

	void AssetManagerPanel::HandleDragDrop(RendererID icon, Ref<Asset>& asset)
	{
		if (asset->Type == AssetType::Directory && m_IsDragging)
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
						Ref<Asset> droppedAsset = AssetManager::GetAsset<Asset>(handle, false);
						
						bool result = FileSystem::MoveFile(droppedAsset->FilePath, asset->FilePath);
						if (result)
							droppedAsset->ParentDirectory = asset->Handle;
					}

					m_UpdateDirectoryNextFrame = true;
				}
			}
		}

		if (!m_SelectedAssets.IsSelected(asset->Handle) || m_IsDragging)
			return;

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlapped) && ImGui::IsItemClicked(ImGuiMouseButton_Left))
			m_IsDragging = true;

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::Image((ImTextureID)icon, ImVec2(20, 20));
			ImGui::SameLine();
			ImGui::Text(asset->FileName.c_str());
			ImGui::SetDragDropPayload("asset_payload", m_SelectedAssets.GetSelectionData(), m_SelectedAssets.SelectionCount() * sizeof(AssetHandle));
			m_IsDragging = true;
			ImGui::EndDragDropSource();
		}
	}

	void AssetManagerPanel::RenderBreadCrumbs()
	{
		if (ImGui::ImageButton("##btn_back", (ImTextureID)m_BackbtnTex->GetRendererID(), ImVec2(20, 18)))
		{
			if (m_CurrentDirHandle == m_BaseDirectoryHandle) return;
			m_NextDirHandle = m_CurrentDirHandle;
			m_PrevDirHandle = m_CurrentDirectory->ParentDirectory;
			UpdateCurrentDirectory(m_PrevDirHandle);
		}

		ImGui::SameLine();

		if (ImGui::ImageButton("##btn_fwrd", (ImTextureID)m_FwrdbtnTex->GetRendererID(), ImVec2(20, 18)))
		{
			UpdateCurrentDirectory(m_NextDirHandle);
		}

		ImGui::SameLine();

		{
			ImGui::PushItemWidth(200);

			static char s_EmptyBuf[1] = { '\0' };
			char* buf = m_RenamingSelected ? s_EmptyBuf : m_InputBuffer;

			if (ImGui::InputTextWithHint("##search", "Search...", buf, m_RenamingSelected ? 1 : MAX_INPUT_BUFFER_LENGTH))
			{
				if (strlen(m_InputBuffer) == 0)
				{
					UpdateCurrentDirectory(m_CurrentDirHandle);
				}
				else
				{
					m_CurrentDirAssets = AssetManager::SearchFiles(m_InputBuffer, m_CurrentDirectory->FilePath);
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
				Ref<Directory> dirInfo = AssetManager::GetAsset<Directory>(currentHandle);
				m_BreadCrumbData.push_back(dirInfo);
				currentHandle = dirInfo->ParentDirectory;
			}

			std::reverse(m_BreadCrumbData.begin(), m_BreadCrumbData.end());

			m_UpdateBreadCrumbs = false;
		}

		for (int i = 0; i < m_BreadCrumbData.size(); i++)
		{
			if (m_BreadCrumbData[i]->FileName != "assets")
				ImGui::Text("/");

			ImGui::SameLine();

			int size = strlen(m_BreadCrumbData[i]->FileName.c_str()) * 7;

			if (ImGui::Selectable(m_BreadCrumbData[i]->FileName.c_str(), false, 0, ImVec2(size, 22)))
			{
				UpdateCurrentDirectory(m_BreadCrumbData[i]->Handle);
			}

			ImGui::SameLine();
		}

		ImGui::SameLine();

		ImGui::Dummy(ImVec2(ImGui::GetColumnWidth() - 400, 0));
		ImGui::SameLine();
	}

	void AssetManagerPanel::HandleRenaming(Ref<Asset>& asset)
	{
		if (m_SelectedAssets.SelectionCount() > 1)
			return;

		if (!m_RenamingSelected && Input::IsKeyPressed(KeyCode::F2))
		{
			memset(m_InputBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
			memcpy(m_InputBuffer, asset->FileName.c_str(), asset->FileName.size());
			m_RenamingSelected = true;
		}

		if (m_RenamingSelected)
		{
			ImGui::SetKeyboardFocusHere();
			if (ImGui::InputText("##rename_dummy", m_InputBuffer, MAX_INPUT_BUFFER_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				VA_CORE_INFO("Renaming to {0}", m_InputBuffer);
				AssetManager::Rename(asset, m_InputBuffer);
				m_RenamingSelected = false;
				m_SelectedAssets.Clear();
				m_UpdateDirectoryNextFrame = true;
			}
		}
	}

	void AssetManagerPanel::UpdateCurrentDirectory(AssetHandle directoryHandle)
	{
		m_UpdateBreadCrumbs = true;
		m_CurrentDirAssets.clear();
		m_CurrentDirHandle = directoryHandle;
		m_CurrentDirectory = AssetManager::GetAsset<Directory>(m_CurrentDirHandle);
		m_CurrentDirAssets = AssetManager::GetAssetsInDirectory(m_CurrentDirHandle);
	}

}