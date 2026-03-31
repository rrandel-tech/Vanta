#include "vapch.hpp"
#include "AssetManager.hpp"

#include "Renderer/Mesh.hpp"
#include "Renderer/SceneRenderer.hpp"

#include "yaml-cpp/yaml.h"

#include <filesystem>

namespace Vanta {

	static const char* s_AssetRegistryPath = "assets/AssetRegistry.var";

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

		s_AssetRegistry.clear();
		s_LoadedAssets.clear();
	}

	void AssetManager::OnFileSystemChanged(FileSystemChangedEvent e)
	{
		e.NewName = Utils::RemoveExtension(e.NewName);

		s_AssetsChangeCallback(e);

		switch (e.Action)
		{
		case FileSystemAction::Added:
			if (!e.IsDirectory)
				ImportAsset(e.FilePath);
			break;
		case FileSystemAction::Delete:
			RemoveAsset(GetAssetHandleFromFilePath(e.FilePath));
			break;
		case FileSystemAction::Modified:
			// TODO: Reload data if loaded
			break;
		case FileSystemAction::Rename:
		{
			if (e.IsDirectory)
				return;

			std::filesystem::path oldFilePath = e.FilePath;
			oldFilePath = oldFilePath.parent_path() / e.OldName;
			OnAssetRenamed(GetAssetHandleFromFilePath(oldFilePath.string()), e.FilePath);

			s_AssetsChangeCallback(e);
			break;
		}
		}
	}

	// NOTE: Most likely temporary
	static AssetMetadata s_NullMetadata;

	AssetMetadata& AssetManager::GetMetadata(AssetHandle handle)
	{
		for (auto&[filepath, metadata] : s_AssetRegistry)
		{
			if (metadata.Handle == handle)
				return metadata;
		}

		return s_NullMetadata;
	}

	AssetMetadata& AssetManager::GetMetadata(const std::string& filepath)
	{
		std::string fixedFilePath = filepath;
		std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');

		if (s_AssetRegistry.find(fixedFilePath) != s_AssetRegistry.end())
			return s_AssetRegistry[fixedFilePath];

		return s_NullMetadata;
	}

	AssetHandle AssetManager::GetAssetHandleFromFilePath(const std::string& filepath)
	{
		std::string fixedFilepath = filepath;
		std::replace(fixedFilepath.begin(), fixedFilepath.end(), '\\', '/');
		
		if (s_AssetRegistry.find(fixedFilepath) != s_AssetRegistry.end())
			return s_AssetRegistry[fixedFilepath].Handle;

		return 0;
	}

	void AssetManager::Rename(AssetHandle assetHandle, const std::string& newName)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		std::string newFilePath = FileSystem::Rename(metadata.FilePath, newName);
		OnAssetRenamed(assetHandle, newFilePath);
	}

	void AssetManager::OnAssetRenamed(AssetHandle assetHandle, const std::string& newFilePath)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		s_AssetRegistry.erase(metadata.FilePath);
		metadata.FilePath = newFilePath;
		metadata.FileName = Utils::RemoveExtension(Utils::GetFilename(newFilePath));
		s_AssetRegistry[metadata.FilePath] = metadata;
		WriteRegistryToFile();
	}

	/*void AssetManager::MoveAsset(AssetHandle assetHandle, const std::string& filepath)
	{
		Ref<Asset>& asset = s_LoadedAssets[assetHandle];
		Ref<Directory> directory = s_LoadedAssets[newDirectory].As<Directory>();
		Ref<Directory> currentDirectory = s_LoadedAssets[asset->ParentDirectory].As<Directory>();
		AssetMetadata metadata = s_AssetRegistry[asset->FilePath];

		bool result = FileSystem::MoveFile(asset->FilePath, directory->FilePath);

		if (!result)
			return;

		asset->FilePath = directory->FilePath + "/" + asset->FileName + "." + asset->Extension;

		s_AssetRegistry.erase(metadata.FilePath);
		metadata.FilePath = asset->FilePath;

		s_AssetRegistry[metadata.FilePath] = metadata;

		asset->ParentDirectory = currentDirectory->Handle;
		directory->Assets.push_back(assetHandle);
		currentDirectory->Assets.erase(std::remove(currentDirectory->Assets.begin(), currentDirectory->Assets.end(), assetHandle), currentDirectory->Assets.end());

		WriteRegistryToFile();
	}*/

	void AssetManager::RemoveAsset(AssetHandle assetHandle)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		s_AssetRegistry.erase(metadata.FilePath);
		s_LoadedAssets.erase(assetHandle);

		WriteRegistryToFile();
	}

	AssetType AssetManager::GetAssetTypeForFileType(const std::string& extension)
	{
		if (extension == "vscene") return AssetType::Scene;
		if (extension == "fbx") return AssetType::MeshAsset;
		if (extension == "obj") return AssetType::MeshAsset;
		if (extension == "vam") return AssetType::Mesh;
		if (extension == "png") return AssetType::Texture;
		if (extension == "hdr") return AssetType::EnvMap;
		if (extension == "wav") return AssetType::Audio;
		if (extension == "ogg") return AssetType::Audio;
		return AssetType::None;
	}

	void AssetManager::LoadAssetRegistry()
	{
		if (!FileSystem::Exists(s_AssetRegistryPath))
			return;

		std::ifstream stream(s_AssetRegistryPath);
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
			AssetMetadata metadata;
			metadata.Handle = entry["Handle"].as<uint64_t>();
			metadata.FilePath = entry["FilePath"].as<std::string>();
			metadata.FileName = Utils::RemoveExtension(Utils::GetFilename(metadata.FilePath));
			metadata.Extension = Utils::GetExtension(Utils::GetFilename(metadata.FilePath));
			metadata.Type = (AssetType)Utils::AssetTypeFromString(entry["Type"].as<std::string>());

			// TODO: Improve this
			if (metadata.Type == AssetType::None)
				continue;

			if (!FileSystem::Exists(metadata.FilePath))
			{
				VA_CORE_WARN("Missing asset '{0}' detected in registry file, trying to locate...", metadata.FilePath);

				std::string mostLikelyCandiate;
				uint32_t bestScore = 0;

				for (auto& pathEntry : std::filesystem::recursive_directory_iterator("assets/"))
				{
					const std::filesystem::path& path = pathEntry.path();

					if (path.filename() != Utils::GetFilename(metadata.FilePath))
						continue;

					if (bestScore > 0)
						VA_CORE_WARN("Multiple candiates found...");

					std::vector<std::string> candiateParts = Utils::SplitString(path.string(), "/\\");

					uint32_t score = 0;
					for (const auto& part : candiateParts)
					{
						if (metadata.FilePath.find(part) != std::string::npos)
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
					VA_CORE_ERROR("Failed to locate a potential match for '{0}'", metadata.FilePath);
					continue;
				}

				metadata.FilePath = mostLikelyCandiate;
				std::replace(metadata.FilePath.begin(), metadata.FilePath.end(), '\\', '/');
				VA_CORE_WARN("Found most likely match '{0}'", metadata.FilePath);
			}

			if (metadata.Handle == 0)
			{
				VA_CORE_WARN("AssetHandle for {0} is 0, this shouldn't happen.", metadata.FilePath);
				continue;
			}

			s_AssetRegistry[metadata.FilePath] = metadata;
		}
	}

	AssetHandle AssetManager::ImportAsset(const std::string& filepath)
	{
		std::string fixedFilePath = filepath;
		std::replace(fixedFilePath.begin(), fixedFilePath.end(), '\\', '/');

		// Already in the registry
		if (s_AssetRegistry.find(fixedFilePath) != s_AssetRegistry.end())
			return 0;

		AssetType type = GetAssetTypeForFileType(Utils::GetExtension(fixedFilePath));

		// TODO: Improve this
		if (type == AssetType::None)
			return 0;

		AssetMetadata metadata;
		metadata.Handle = AssetHandle();
		metadata.FilePath = fixedFilePath;
		metadata.FileName = Utils::RemoveExtension(Utils::GetFilename(fixedFilePath));
		metadata.Extension = Utils::GetExtension(fixedFilePath);
		metadata.Type = type;
		s_AssetRegistry[fixedFilePath] = metadata;

		return metadata.Handle;
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
		ProcessDirectory("assets");
		WriteRegistryToFile();
	}

	void AssetManager::WriteRegistryToFile()
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::BeginSeq;
		for (auto& [filepath, metadata] : s_AssetRegistry)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << metadata.Handle;
			out << YAML::Key << "FilePath" << YAML::Value << metadata.FilePath;
			out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(metadata.Type);
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(s_AssetRegistryPath);
		fout << out.c_str();
	}

	std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::s_LoadedAssets;
	std::unordered_map<std::string, AssetMetadata> AssetManager::s_AssetRegistry;
	AssetManager::AssetsChangeEventFn AssetManager::s_AssetsChangeCallback;

}