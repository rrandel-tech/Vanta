#include "vapch.hpp"
#include "MeshSerializer.hpp"

#include "yaml-cpp/yaml.h"

#include "Asset/AssetManager.hpp"

namespace Vanta {

    MeshSerializer::MeshSerializer()
    {
    }

    bool MeshSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
    {
        // TODO: this needs to open up a Vanta Mesh file and make sure
        //       the MeshAsset file is also loaded
        VA_CORE_ASSERT(false);
        return false;
    }

    void MeshSerializer::Serialize(const std::string& filepath)
    {
#if 0
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Mesh";
        {
            out << YAML::BeginMap;
            out << YAML::Key << "AssetHandle";
            out << YAML::Value << m_Mesh->Handle;
            out << YAML::Key << "MeshAsset";
            out << YAML::Value << m_Mesh->GetMeshAsset()->Handle;
            out << YAML::Key << "SubmeshIndices";
            out << YAML::Flow;
            out << YAML::Value << m_Mesh->GetSubmeshes();
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        VA_CORE_WARN("Serializing to {0}", filepath);
        std::ofstream fout(filepath);
        VA_CORE_ASSERT(fout.good());
        if (fout.good())
            fout << out.c_str();
#endif
    }

    void MeshSerializer::SerializeRuntime(const std::string& filepath)
    {
        VA_CORE_ASSERT(false);
    }

    bool MeshSerializer::Deserialize(const std::string& filepath)
    {
#if 0
        std::ifstream stream(filepath);
        VA_CORE_ASSERT(stream);
        std::stringstream strStream;
        strStream << stream.rdbuf();

        YAML::Node data = YAML::Load(strStream.str());
        if (!data["Mesh"])
            return false;

        YAML::Node rootNode = data["Mesh"];
        if (!rootNode["MeshAsset"])
            return false;

        AssetHandle assetHandle = rootNode["MeshAsset"].as<uint64_t>();
        Ref<MeshAsset> meshAsset = AssetManager::GetAsset<MeshAsset>(assetHandle);
        m_Mesh->SetMeshAsset(meshAsset);
        m_Mesh->SetSubmeshes(rootNode["Submeshes"].as < std::vector<uint32_t>());
#endif
        return false;
    }

    bool MeshSerializer::DeserializeRuntime(const std::string& filepath)
    {
        VA_CORE_ASSERT(false);
        return false;
    }

}