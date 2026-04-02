#pragma once

#include "Project/Project.hpp"
#include "Utilities/FileSystem.hpp"
#include "Utilities/StringUtils.hpp"

#include "AssetImporter.hpp"
#include "AssetRegistry.hpp"

#include <map>
#include <unordered_map>

namespace Vanta {

	// Deserialized from project file - these are just defaults
	class AssetManagerConfig
	{
		std::string AssetDirectory = "Assets/";
		std::string AssetRegistryPath = "Assets/AssetRegistry.var";

		std::string MeshPath = "Assets/Meshes/";
		std::string MeshSourcePath = "Assets/Meshes/Source/";
	};

	class AssetManager
	{
	public:
		using AssetsChangeEventFn = std::function<void(FileSystemChangedEvent)>;
	public:
		static void Init();
		static void SetAssetChangeCallback(const AssetsChangeEventFn& callback);
		static void Shutdown();

		static AssetMetadata& GetMetadata(AssetHandle handle);
		static AssetMetadata& GetMetadata(const std::string& filepath);

		static std::string GetFileSystemPath(const AssetMetadata& metadata) { return (Project::GetAssetDirectory() / metadata.FilePath).string(); }
		static std::string GetRelativePath(const std::string& filepath);

		static AssetHandle GetAssetHandleFromFilePath(const std::string& filepath);
		static bool IsAssetHandleValid(AssetHandle assetHandle) { return GetMetadata(assetHandle).IsValid(); }

		static AssetType GetAssetTypeFromExtension(const std::string& extension);
		static AssetType GetAssetTypeFromPath(const std::filesystem::path& path);

		static AssetHandle ImportAsset(const std::string& filepath);
		static bool ReloadData(AssetHandle assetHandle);

		template<typename T, typename... Args>
		static Ref<T> CreateNewAsset(const std::string& filename, const std::string& directory, Args&&... args)
		{
			static_assert(std::is_base_of<Asset, T>::value, "CreateNewAsset only works for types derived from Asset");

			// TODO: this shouldn't be needed anymore - all paths MUST be relative to asset directory anyway
			std::filesystem::path relativePath = std::filesystem::relative(std::filesystem::path(directory), Project::GetAssetDirectory());
			std::string directoryPath = relativePath.string();
			std::replace(directoryPath.begin(), directoryPath.end(), '\\', '/');

			AssetMetadata metadata;
			metadata.Handle = AssetHandle();
			metadata.FilePath = directoryPath + "/" + filename;
			metadata.IsDataLoaded = true;
			metadata.Type = T::GetStaticType();

			if (FileExists(metadata))
			{
				bool foundAvailableFileName = false;
				int current = 1;

				while (!foundAvailableFileName)
				{
					std::string nextFilePath = (relativePath / metadata.FilePath.stem()).string();
					if (current < 10)
						nextFilePath += " (0" + std::to_string(current) + ")";
					else
						nextFilePath += " (" + std::to_string(current) + ")";
					nextFilePath += metadata.FilePath.extension().string();

					if (!FileSystem::Exists(nextFilePath))
					{
						foundAvailableFileName = true;
						metadata.FilePath = nextFilePath;
						break;
					}

					current++;
				}
			}

			s_AssetRegistry[metadata.FilePath.string()] = metadata;

			WriteRegistryToFile();

			Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
			asset->Handle = metadata.Handle;
			s_LoadedAssets[asset->Handle] = asset;
			AssetImporter::Serialize(metadata, asset);

			return asset;
		}

		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			auto& metadata = GetMetadata(assetHandle);

			Ref<Asset> asset = nullptr;
			if (!metadata.IsDataLoaded)
			{
				metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
				if (!metadata.IsDataLoaded)
					return nullptr;

				s_LoadedAssets[assetHandle] = asset;
			}
			else
			{
				asset = s_LoadedAssets[assetHandle];
			}

			return asset.As<T>();
		}

		template<typename T>
		static Ref<T> GetAsset(const std::string& filepath)
		{
			std::string fp = filepath;
			if (fp.find(Project::GetAssetDirectory().string()) == std::string::npos)
				fp = (Project::GetAssetDirectory() / fp).string();

			return GetAsset<T>(GetAssetHandleFromFilePath(fp));
		}

		static bool FileExists(AssetMetadata& metadata)
		{
			return FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / metadata.FilePath);
		}

		static void OnImGuiRender(bool& open);
	private:
		static void LoadAssetRegistry();
		static void ProcessDirectory(const std::string& directoryPath);
		static void ReloadAssets();
		static void WriteRegistryToFile();

		static void OnFileSystemChanged(FileSystemChangedEvent e);
		static void OnAssetRenamed(AssetHandle assetHandle, const std::string& newFilePath);
		static void OnAssetMoved(AssetHandle assetHandle, const std::string& destinationPath);
		static void OnAssetDeleted(AssetHandle assetHandle);

	private:
		static std::unordered_map<AssetHandle, Ref<Asset>> s_LoadedAssets;
		static AssetsChangeEventFn s_AssetsChangeCallback;
		inline static AssetRegistry s_AssetRegistry;
	private:
		friend class ContentBrowserPanel;
		friend class ContentBrowserAsset;
	};

}