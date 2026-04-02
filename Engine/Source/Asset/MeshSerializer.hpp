#pragma once

#include "Asset/AssetSerializer.hpp"
#include "Renderer/Mesh.hpp"

namespace Vanta {

    class MeshSerializer : public AssetSerializer
    {
    public:
        MeshSerializer();

        void Serialize(Ref<Mesh> mesh, const std::string& filepath);
        void SerializeRuntime(Ref<Mesh> mesh, const std::string& filepath);

        bool Deserialize(const std::string& filepath);
        bool DeserializeRuntime(const std::string& filepath);

        virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
        virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
    };

}