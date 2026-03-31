#include "vapch.hpp"
#include "AssetSerializer.hpp"
#include "Utilities/StringUtils.hpp"
#include "Utilities/FileSystem.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/Renderer.hpp"

#include "yaml-cpp/yaml.h"

namespace Vanta {

	void AssetSerializer::CopyMetadata(const Ref<Asset>& from, Ref<Asset>& to) const
	{
		to->Handle = from->Handle;
		to->FilePath = from->FilePath;
		to->FileName = from->FileName;
		to->Extension = from->Extension;
		to->ParentDirectory = from->ParentDirectory;
		to->Type = from->Type;
		to->IsDataLoaded = true;
	}

	bool TextureSerializer::TryLoadData(Ref<Asset>& asset) const
	{
		Ref<Asset> temp = asset;
		asset = Texture2D::Create(asset->FilePath);
		CopyMetadata(temp, asset);
		return (asset.As<Texture2D>())->Loaded();
	}

	bool MeshSerializer::TryLoadData(Ref<Asset>& asset) const
	{
		Ref<Asset> temp = asset;
		asset = Ref<Mesh>::Create(asset->FilePath);
		CopyMetadata(temp, asset);
		return (asset.As<Mesh>())->GetStaticVertices().size() > 0; // Maybe?
	}

	bool EnvironmentSerializer::TryLoadData(Ref<Asset>& asset) const
	{
		auto [radiance, irradiance] = Renderer::CreateEnvironmentMap(asset->FilePath);

		if (!radiance || !irradiance)
			return false;

		Ref<Asset> temp = asset;
		asset = Ref<Environment>::Create(radiance, irradiance);
		CopyMetadata(temp, asset);
		return true;
	}

}