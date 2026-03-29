#include "vapch.hpp"
#include "AssetManager.hpp"

#include "Renderer/Mesh.hpp"
#include "Renderer/SceneRenderer.hpp"

#include "yaml-cpp/yaml.h"

#include <filesystem>

namespace Vanta {

	void AssetTypes::Init()
	{
		s_Types["vscene"] = AssetType::Scene;
		s_Types["fbx"] = AssetType::Mesh;
		s_Types["obj"] = AssetType::Mesh;
		s_Types["blend"] = AssetType::Mesh;
		s_Types["png"] = AssetType::Texture;
		s_Types["hdr"] = AssetType::EnvMap;
		s_Types["wav"] = AssetType::Audio;
		s_Types["ogg"] = AssetType::Audio;
		s_Types["lua"] = AssetType::Script;
	}

	AssetType AssetTypes::GetAssetTypeFromExtension(const std::string& extension)
	{
		return s_Types.find(extension) != s_Types.end() ? s_Types[extension] : AssetType::Other;
	}

	std::map<std::string, AssetType> AssetTypes::s_Types;

	void AssetManager::Init()
	{
		FileSystem::SetChangeCallback(AssetManager::OnFileSystemChanged);
		ReloadAssets();
	}

	void AssetManager::SetAssetChangeCallback(const AssetsChangeEventFn& callback)
	{
		s_AssetsChangeCallback = callback;
	}

	void AssetManager::Shutdown()
	{
		s_LoadedAssets.clear();
	}

	std::vector<Ref<Asset>> AssetManager::GetAssetsInDirectory(AssetHandle directoryHandle)
	{
		std::vector<Ref<Asset>> results;

		for (auto& asset : s_LoadedAssets)
		{
			if (asset.second && asset.second->ParentDirectory == directoryHandle && asset.second->Handle != directoryHandle)
				results.push_back(asset.second);
		}

		return results;
	}

	// Utility function to find the parent of an unprocessed directory
	AssetHandle AssetManager::FindParentHandleInChildren(Ref<Directory>& dir, const std::string& dirName)
	{
		if (dir->FileName == dirName)
			return dir->Handle;

		for (AssetHandle childHandle : dir->ChildDirectories)
		{
			Ref<Directory> child = GetAsset<Directory>(childHandle);
			AssetHandle dirHandle = FindParentHandleInChildren(child, dirName);

			if (IsAssetHandleValid(dirHandle))
				return dirHandle;
		}

		return 0;
	}

	AssetHandle AssetManager::FindParentHandle(const std::string& filepath)
	{
		std::vector<std::string> parts = Utils::SplitString(filepath, "/\\");
		std::string parentFolder = parts[parts.size() - 2];
		Ref<Directory> assetsDirectory = GetAsset<Directory>(GetAssetHandleFromFilePath("assets"));
		return FindParentHandleInChildren(assetsDirectory, parentFolder);
	}

	void AssetManager::OnFileSystemChanged(FileSystemChangedEvent e)
	{
		e.NewName = Utils::RemoveExtension(e.NewName);
		e.OldName = Utils::RemoveExtension(e.OldName);

		AssetHandle parentHandle = FindParentHandle(e.FilePath);

		if (e.Action == FileSystemAction::Added)
		{
			if (e.IsDirectory)
				ProcessDirectory(e.FilePath, parentHandle);
			else
				ImportAsset(e.FilePath, parentHandle);
		}

		if (e.Action == FileSystemAction::Modified)
		{
			if (!e.IsDirectory)
				ImportAsset(e.FilePath, parentHandle);
		}

		if (e.Action == FileSystemAction::Rename)
		{
			for (auto it = s_LoadedAssets.begin(); it != s_LoadedAssets.end(); it++)
			{
				if (it->second->FileName == e.OldName)
				{
					it->second->FilePath = e.FilePath;
					it->second->FileName = e.NewName;
				}
			}
		}

		if (e.Action == FileSystemAction::Delete)
		{
			for (auto it = s_LoadedAssets.begin(); it != s_LoadedAssets.end(); it++)
			{
				if (it->second->FilePath != e.FilePath)
					continue;

				RemoveAsset(it->first);
				break;
			}
		}

		s_AssetsChangeCallback();
	}

	std::vector<Ref<Asset>> AssetManager::SearchFiles(const std::string& query, const std::string& searchPath)
	{
		std::vector<Ref<Asset>> results;

		if (!searchPath.empty())
		{
			for (const auto&[key, asset] : s_LoadedAssets)
			{
				if (asset->FileName.find(query) != std::string::npos && asset->FilePath.find(searchPath) != std::string::npos)
				{
					results.push_back(asset);
				}
			}
		}

		return results;
	}

	std::string AssetManager::GetParentPath(const std::string& path)
	{
		return std::filesystem::path(path).parent_path().string();
	}

	bool AssetManager::IsDirectory(const std::string& filepath)
	{
		for (auto&[handle, asset] : s_LoadedAssets)
		{
			if (asset->Type == AssetType::Directory && asset->FilePath == filepath)
				return true;
		}

		return false;
	}

	AssetHandle AssetManager::GetAssetHandleFromFilePath(const std::string& filepath)
	{
		for (auto&[id, asset] : s_LoadedAssets)
		{
			if (asset->FilePath == filepath)
				return id;
		}

		return 0;
	}

	bool AssetManager::IsAssetHandleValid(AssetHandle assetHandle)
	{
		return assetHandle != 0 && s_LoadedAssets.find(assetHandle) != s_LoadedAssets.end();
	}

	void AssetManager::Rename(Ref<Asset>& asset, const std::string& newName)
	{
		std::string newFilePath = FileSystem::Rename(asset->FilePath, newName);
		std::string oldFilePath = asset->FilePath;
		asset->FilePath = newFilePath;
		asset->FileName = newName;

		if (FileSystem::Exists(oldFilePath + ".meta"))
		{
			std::string metaFileName = oldFilePath;

			if (asset->Extension != "")
				metaFileName += "." + asset->Extension;

			FileSystem::Rename(oldFilePath + ".meta", metaFileName);
			AssetSerializer::CreateMetaFile(asset);
		}
	}

	void AssetManager::RemoveAsset(AssetHandle assetHandle)
	{
		Ref<Asset> asset = s_LoadedAssets[assetHandle];
		if (asset->Type == AssetType::Directory)
		{
			if (IsAssetHandleValid(asset->ParentDirectory))
			{
				auto& childList = s_LoadedAssets[asset->ParentDirectory].As<Directory>()->ChildDirectories;
				childList.erase(std::remove(childList.begin(), childList.end(), assetHandle), childList.end());
			}

			for (auto child : asset.As<Directory>()->ChildDirectories)
				RemoveAsset(child);

			for (auto it = s_LoadedAssets.begin(); it != s_LoadedAssets.end(); )
			{
				if (it->second->ParentDirectory != assetHandle)
				{
					it++;
					continue;
				}

				it = s_LoadedAssets.erase(it);
			}
		}

		s_LoadedAssets.erase(assetHandle);
	}

	std::string AssetManager::StripExtras(const std::string& filename)
	{
		std::vector<std::string> out;
		size_t start;
		size_t end = 0;

		while ((start = filename.find_first_not_of(".", end)) != std::string::npos)
		{
			end = filename.find(".", start);
			out.push_back(filename.substr(start, end - start));
		}

		if (out[0].length() >= 10)
		{
			auto cutFilename = out[0].substr(0, 9) + "...";
			return cutFilename;
		}

		auto filenameLength = out[0].length();
		auto paddingToAdd = 9 - filenameLength;

		std::string newFileName;

		for (int i = 0; i <= paddingToAdd; i++)
		{
			newFileName += " ";
		}

		newFileName += out[0];

		return newFileName;
	}

	void AssetManager::ImportAsset(const std::string& filepath, AssetHandle parentHandle)
	{
		std::string extension = Utils::GetExtension(filepath);
		if (extension == "meta")
			return;

		AssetType type = AssetTypes::GetAssetTypeFromExtension(extension);
		Ref<Asset> asset = AssetSerializer::LoadAssetInfo(filepath, parentHandle, type);

		if (s_LoadedAssets.find(asset->Handle) != s_LoadedAssets.end())
		{
			if (s_LoadedAssets[asset->Handle]->IsDataLoaded)
			{
				asset = AssetSerializer::LoadAssetData(asset);
			}
		}

		s_LoadedAssets[asset->Handle] = asset;
	}

	AssetHandle AssetManager::ProcessDirectory(const std::string& directoryPath, AssetHandle parentHandle)
	{
		Ref<Directory> dirInfo = AssetSerializer::LoadAssetInfo(directoryPath, parentHandle, AssetType::Directory).As<Directory>();
		s_LoadedAssets[dirInfo->Handle] = dirInfo;

		if (IsAssetHandleValid(parentHandle))
			s_LoadedAssets[parentHandle].As<Directory>()->ChildDirectories.push_back(dirInfo->Handle);

		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
				ProcessDirectory(entry.path().string(), dirInfo->Handle);
			else
				ImportAsset(entry.path().string(), dirInfo->Handle);
		}

		return dirInfo->Handle;
	}

	void AssetManager::ReloadAssets()
	{
		ProcessDirectory("assets", 0);
	}

	std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::s_LoadedAssets;
	AssetManager::AssetsChangeEventFn AssetManager::s_AssetsChangeCallback;

}