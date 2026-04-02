#include "vapch.hpp"
#include "AssetImporter.hpp"
#include "AssetManager.hpp"
#include "MeshSerializer.hpp"

namespace Vanta {

    void AssetImporter::Init()
    {
        s_Serializers[AssetType::Texture] = CreateScope<TextureSerializer>();
        s_Serializers[AssetType::MeshAsset] = CreateScope<MeshAssetSerializer>();
        s_Serializers[AssetType::Mesh] = CreateScope<MeshSerializer>();
        s_Serializers[AssetType::EnvMap] = CreateScope<EnvironmentSerializer>();
    }

    void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
    {
        if (s_Serializers.find(metadata.Type) == s_Serializers.end())
        {
            VA_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.extension().string());
            return;
        }

        s_Serializers[asset->GetAssetType()]->Serialize(metadata, asset);
    }


    void AssetImporter::Serialize(const Ref<Asset>& asset)
    {
        const AssetMetadata& metadata = AssetManager::GetMetadata(asset->Handle);
        Serialize(metadata, asset);
    }

    bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset)
    {
        if (s_Serializers.find(metadata.Type) == s_Serializers.end())
        {
            VA_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.extension().string());
            return false;
        }

        return s_Serializers[metadata.Type]->TryLoadData(metadata, asset);
    }

    std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_Serializers;

}