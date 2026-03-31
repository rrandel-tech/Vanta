#pragma once

#include "AssetMetadata.hpp"

namespace Vanta {

    class AssetSerializer
    {
    public:
        virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
        virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;
    };

    class TextureSerializer : public AssetSerializer
    {
    public:
        virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override{}
        virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class MeshAssetSerializer : public AssetSerializer
    {
    public:
        virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override{}
        virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

    class EnvironmentSerializer : public AssetSerializer
    {
    public:
        virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override{}
        virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

}