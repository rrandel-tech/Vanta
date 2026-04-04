#include "vapch.hpp"
#include "AssetManager.hpp"

#include "Renderer/Mesh.hpp"
#include "Renderer/SceneRenderer.hpp"
#include "Project/Project.hpp"
#include "ImGui/ImGui.hpp"

#include "yaml-cpp/yaml.h"

#include <filesystem>

#include "AssetExtensions.hpp"

namespace Vanta {

	void AssetManager::Init()
	{
		AssetImporter::Init();

		LoadAssetRegistry();
		FileSystem::SetChangeCallback(AssetManager::OnFileSystemChanged);
		ReloadAssets();
		WriteRegistryToFile();
	}

	void AssetManager::SetAssetChangeCallback(const AssetsChangeEventFn& callback)
	{
		s_AssetsChangeCallback = callback;
	}

	void AssetManager::Shutdown()
	{
		WriteRegistryToFile();

		s_AssetRegistry.Clear();
		s_LoadedAssets.clear();
	}

	void AssetManager::OnFileSystemChanged(FileSystemChangedEvent e)
	{
		e.FilePath = (Project::GetAssetDirectory() / e.FilePath).string();
		std::string temp = e.FilePath.string();
		std::replace(temp.begin(), temp.end(), '\\', '/');
		e.FilePath = temp;

		e.NewName = Utils::RemoveExtension(e.NewName);

		if (!e.IsDirectory)
		{
			switch (e.Action)
			{
			case FileSystemAction::Added:
				ImportAsset(e.FilePath.string());
				break;
			case FileSystemAction::Delete:
				OnAssetDeleted(GetAssetHandleFromFilePath(e.FilePath.string()));
				break;
			case FileSystemAction::Modified:
				// TODO: Reload data if loaded
				break;
			case FileSystemAction::Rename:
			{
				AssetType previousType = GetAssetTypeFromPath(e.OldName);
				AssetType newType = GetAssetTypeFromPath(e.FilePath);

				if (previousType == AssetType::None && newType != AssetType::None)
				{
					ImportAsset(e.FilePath.string());
				}
				else
				{
					OnAssetRenamed(GetAssetHandleFromFilePath((e.FilePath.parent_path() / e.OldName).string()), e.FilePath.string());
					e.WasTracking = true;
				}
				break;
			}
			}
		}

		s_AssetsChangeCallback(e);
	}

	// NOTE: Most likely temporary
	static AssetMetadata s_NullMetadata;

	AssetMetadata& AssetManager::GetMetadata(AssetHandle handle)
	{
		for (auto& [filepath, metadata] : s_AssetRegistry)
		{
			if (metadata.Handle == handle)
				return metadata;
		}

		return s_NullMetadata;
	}

	AssetMetadata& AssetManager::GetMetadata(const std::string& filepath)
	{
		if (s_AssetRegistry.Contains(filepath))
			return s_AssetRegistry[filepath];

		return s_NullMetadata;
	}

	std::string AssetManager::GetRelativePath(const std::string& filepath)
	{
		std::string result = filepath;
		if (filepath.find(Project::GetActive()->GetAssetDirectory().string()) != std::string::npos)
			result = std::filesystem::relative(result, Project::GetActive()->GetAssetDirectory()).string();
		std::replace(result.begin(), result.end(), '\\', '/');
		return result;
	}

	AssetHandle AssetManager::GetAssetHandleFromFilePath(const std::string& filepath)
	{
		std::filesystem::path path = filepath;
		if (s_AssetRegistry.Contains(path))
			return s_AssetRegistry.Get(path).Handle;

		return 0;
	}

	void AssetManager::OnAssetRenamed(AssetHandle assetHandle, const std::string& newFilePath)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		s_AssetRegistry.Remove(metadata.FilePath);
		metadata.FilePath = newFilePath;
		s_AssetRegistry[metadata.FilePath] = metadata;
		WriteRegistryToFile();
	}

	// Moving the actual asset file isn't the AssetManagers job
	void AssetManager::OnAssetMoved(AssetHandle assetHandle, const std::string& destinationPath)
	{
		AssetMetadata assetInfo = GetMetadata(assetHandle);

		s_AssetRegistry.Remove(assetInfo.FilePath);
		assetInfo.FilePath = destinationPath / assetInfo.FilePath.filename();
		s_AssetRegistry[assetInfo.FilePath] = assetInfo;

		WriteRegistryToFile();
	}

	void AssetManager::OnAssetDeleted(AssetHandle assetHandle)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		s_AssetRegistry.Remove(metadata.FilePath);
		s_LoadedAssets.erase(assetHandle);

		WriteRegistryToFile();
	}


	AssetType AssetManager::GetAssetTypeFromExtension(const std::string& extension)
	{
		std::string ext = Utils::String::ToLowerCopy(extension);
		if (s_AssetExtensionMap.find(ext) == s_AssetExtensionMap.end())
			return AssetType::None;

		return s_AssetExtensionMap.at(ext.c_str());
	}

	AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	void AssetManager::LoadAssetRegistry()
	{
		const std::string& assetRegistryPath = Project::GetAssetRegistryPath().string();
		if (!FileSystem::Exists(assetRegistryPath))
			return;

		std::ifstream stream(assetRegistryPath);
		VA_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		auto handles = data["Assets"];
		if (!handles)
		{
			VA_CORE_ERROR("AssetRegistry appears to be corrupted!");
			return;
		}

		for (auto entry : handles)
		{
			std::string filepath = entry["FilePath"].as<std::string>();

			AssetMetadata metadata;
			metadata.Handle = entry["Handle"].as<uint64_t>();
			metadata.FilePath = filepath;
			metadata.Type = (AssetType)Utils::AssetTypeFromString(entry["Type"].as<std::string>());

			if (metadata.Type == AssetType::None)
				continue;

			if (!FileSystem::Exists(AssetManager::GetFileSystemPath(metadata)))
			{
				VA_CORE_WARN("Missing asset '{0}' detected in registry file, trying to locate...", metadata.FilePath.string());

				std::string mostLikelyCandiate;
				uint32_t bestScore = 0;

				for (auto& pathEntry : std::filesystem::recursive_directory_iterator(Project::GetAssetDirectory()))
				{
					const std::filesystem::path& path = pathEntry.path();

					if (path.filename() != metadata.FilePath.filename())
						continue;

					if (bestScore > 0)
						VA_CORE_WARN("Multiple candiates found...");

					std::vector<std::string> candiateParts = Utils::SplitString(path.string(), "/\\");

					uint32_t score = 0;
					for (const auto& part : candiateParts)
					{
						if (filepath.find(part) != std::string::npos)
							score++;
					}

					VA_CORE_WARN("'{0}' has a score of {1}, best score is {2}", path.string(), score, bestScore);

					if (bestScore > 0 && score == bestScore)
					{
						// TODO: How do we handle this?
					}

					if (score <= bestScore)
						continue;

					bestScore = score;
					mostLikelyCandiate = path.string();
				}

				if (mostLikelyCandiate.empty() && bestScore == 0)
				{
					VA_CORE_ERROR("Failed to locate a potential match for '{0}'", metadata.FilePath.string());
					continue;
				}

				std::replace(mostLikelyCandiate.begin(), mostLikelyCandiate.end(), '\\', '/');
				metadata.FilePath = std::filesystem::relative(mostLikelyCandiate, Project::GetActive()->GetAssetDirectory());
				VA_CORE_WARN("Found most likely match '{0}'", metadata.FilePath.string());
			}

			if (metadata.Handle == 0)
			{
				VA_CORE_WARN("AssetHandle for {0} is 0, this shouldn't happen.", metadata.FilePath.string());
				continue;
			}

			s_AssetRegistry[metadata.FilePath.string()] = metadata;
		}
	}

	AssetHandle AssetManager::ImportAsset(const std::string& filepath)
	{
		std::filesystem::path path = std::filesystem::relative(filepath, Project::GetAssetDirectory());

		// Already in the registry
		if (s_AssetRegistry.Contains(path))
			return 0; // TODO: should this return the existing asset handle?

		AssetType type = GetAssetTypeFromPath(path);
		if (type == AssetType::None)
			return 0;

		AssetMetadata metadata;
		metadata.Handle = AssetHandle();
		metadata.FilePath = path;
		metadata.Type = type;
		s_AssetRegistry[metadata.FilePath] = metadata;

		return metadata.Handle;
	}

	bool AssetManager::ReloadData(AssetHandle assetHandle)
	{
		auto& metadata = GetMetadata(assetHandle);
		if (!metadata.IsDataLoaded) // Data
			VA_CORE_WARN("Trying to reload asset that was never loaded");

		VA_CORE_ASSERT(s_LoadedAssets.find(assetHandle) != s_LoadedAssets.end());
		Ref<Asset>& asset = s_LoadedAssets.at(assetHandle);
		metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
		return metadata.IsDataLoaded;
	}

	void AssetManager::ProcessDirectory(const std::string& directoryPath)
	{
		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
				ProcessDirectory(entry.path().string());
			else
				ImportAsset(entry.path().string());
		}
	}

	void AssetManager::ReloadAssets()
	{
		ProcessDirectory(Project::GetAssetDirectory().string());
		WriteRegistryToFile();
	}

	void AssetManager::WriteRegistryToFile()
	{
		// Sort assets by UUID to make project managment easier
		struct AssetRegistryEntry
		{
			std::string FilePath;
			AssetType Type;
		};
		std::map<UUID, AssetRegistryEntry> sortedMap;
		for (auto& [filepath, metadata] : s_AssetRegistry)
		{
			std::string pathToSerialize = metadata.FilePath.string();
			// NOTE(Yan): if Windows
			std::replace(pathToSerialize.begin(), pathToSerialize.end(), '\\', '/');
			sortedMap[metadata.Handle] = { pathToSerialize, metadata.Type };
			VA_CORE_ASSERT(pathToSerialize.find("Sandbox") == std::string::npos);
		}

		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::BeginSeq;
		for (auto& [handle, entry] : sortedMap)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "FilePath" << YAML::Value << entry.FilePath;
			out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(entry.Type);
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		const std::string& assetRegistryPath = Project::GetAssetRegistryPath().string();
		std::ofstream fout(assetRegistryPath);
		fout << out.c_str();
	}

	void AssetManager::OnImGuiRender(bool& open)
	{
		if (!open)
			return;

		ImGui::Begin("Asset Manager", &open);
		if (UI::BeginTreeNode("Registry"))
		{
			static char searchBuffer[256];
			ImGui::InputText("##regsearch", searchBuffer, 256);
			UI::BeginPropertyGrid();
			static float columnWidth = 0.0f;
			if (columnWidth == 0.0f)
			{
				ImVec2 textSize = ImGui::CalcTextSize("File Path");
				columnWidth = textSize.x * 2.0f;
				ImGui::SetColumnWidth(0, columnWidth);
			}
			for (const auto& [path, metadata] : s_AssetRegistry)
			{
				std::string handle = std::format("{0}", (uint64_t)metadata.Handle);
				std::string filepath = metadata.FilePath.string();
				std::string type = Utils::AssetTypeToString(metadata.Type);
				if (searchBuffer[0] != 0)
				{
					std::string searchString = searchBuffer;
					Utils::String::ToLower(searchString);
					if (Utils::String::ToLowerCopy(handle).find(searchString) != std::string::npos
						|| Utils::String::ToLowerCopy(filepath).find(searchString) != std::string::npos
						|| Utils::String::ToLowerCopy(type).find(searchString) != std::string::npos)
					{
						UI::Property("Handle", (const std::string&)handle);
						UI::Property("File Path", (const std::string&)filepath);
						UI::Property("Type", (const std::string&)type);
						UI::Separator();
					}
				}
				else
				{
					UI::Property("Handle", (const std::string&)std::format("{0}", (uint64_t)metadata.Handle));
					UI::Property("File Path", (const std::string&)metadata.FilePath.string());
					UI::Property("Type", (const std::string&)Utils::AssetTypeToString(metadata.Type));
					UI::Separator();
				}
			}
			UI::EndPropertyGrid();
			UI::EndTreeNode();
		}
		ImGui::End();
	}

	std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::s_LoadedAssets;
	AssetManager::AssetsChangeEventFn AssetManager::s_AssetsChangeCallback;

}