#include "vapch.hpp"
#include "AssetSerializer.hpp"
#include "Utilities/StringUtils.hpp"
#include "Utilities/FileSystem.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/SceneRenderer.hpp"

#include "yaml-cpp/yaml.h"

namespace Vanta {

	void AssetSerializer::SerializeAsset(const Ref<Asset>& asset, AssetType type)
	{
	}

	Ref<Asset> AssetSerializer::LoadAssetInfo(const std::string& filepath, AssetHandle parentHandle, AssetType type)
	{
		Ref<Asset> asset = Ref<Asset>::Create();

		if (type == AssetType::Directory)
			asset = Ref<Directory>::Create();

		std::string extension = Utils::GetExtension(filepath);
		asset->FilePath = filepath;
		std::replace(asset->FilePath.begin(), asset->FilePath.end(), '\\', '/');

		bool hasMeta = FileSystem::Exists(asset->FilePath + ".meta");
		if (hasMeta)
		{
			LoadMetaData(asset);
		}
		else
		{
			asset->Handle = AssetHandle();
			asset->Type = type;
		}

		asset->Extension = extension;
		asset->FileName = Utils::RemoveExtension(Utils::GetFilename(filepath));
		asset->ParentDirectory = parentHandle;
		asset->IsDataLoaded = false;

		if (!hasMeta)
			CreateMetaFile(asset);

		return asset;
	}

	Ref<Asset> AssetSerializer::LoadAssetData(Ref<Asset>& asset)
	{
		if (asset->Type == AssetType::Directory)
			return asset;

		Ref<Asset> temp = asset;
		bool loadYAMLData = true;

		switch (asset->Type)
		{
			case AssetType::Mesh:
			{
				if (asset->Extension != "blend")
					asset = Ref<Mesh>::Create(asset->FilePath);
				loadYAMLData = false;
				break;
			}
			case AssetType::Texture:
			{
				asset = Texture2D::Create(asset->FilePath);
				loadYAMLData = false;
				break;
			}
			case AssetType::EnvMap:
			{
				auto [radiance, irradiance] = SceneRenderer::CreateEnvironmentMap(asset->FilePath);
				asset = Ref<Environment>::Create(radiance, irradiance);
				loadYAMLData = false;
				break;
			}
			case AssetType::Scene:
			case AssetType::Audio:
			case AssetType::Script:
			case AssetType::Other:
			{
				loadYAMLData = false;
				break;
			}
		}

		if (loadYAMLData)
		{
			asset = DeserializeYAML(asset);
			VA_CORE_ASSERT(asset, "Failed to load asset");
		}

		asset->Handle = temp->Handle;
		asset->FilePath = temp->FilePath;
		asset->FileName = temp->FileName;
		asset->Extension = temp->Extension;
		asset->ParentDirectory = temp->ParentDirectory;
		asset->Type = temp->Type;
		asset->IsDataLoaded = true;

		return asset;
	}

	Ref<Asset> AssetSerializer::DeserializeYAML(const Ref<Asset>& asset)
	{
		return nullptr;
	}

	void AssetSerializer::LoadMetaData(Ref<Asset>& asset)
	{
		std::ifstream stream(asset->FilePath + ".meta");
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Asset"])
			VA_CORE_ASSERT("Invalid File Format");

		asset->Handle = data["Asset"].as<uint64_t>();
		asset->FilePath = data["FilePath"].as<std::string>();
		asset->Type = (AssetType)data["Type"].as<int>();

		if (asset->FileName == "assets" && asset->Handle == 0)
		{
			asset->Handle = AssetHandle();
			CreateMetaFile(asset);
		}
	}

	void AssetSerializer::CreateMetaFile(const Ref<Asset>& asset)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Asset" << YAML::Value << asset->Handle;
		out << YAML::Key << "FilePath" << YAML::Value << asset->FilePath;
		out << YAML::Key << "Type" << YAML::Value << (int)asset->Type;
		out << YAML::EndMap;

		std::ofstream fout(asset->FilePath + ".meta");
		fout << out.c_str();
	}

}