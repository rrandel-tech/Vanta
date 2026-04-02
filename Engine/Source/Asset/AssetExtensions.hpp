#pragma once

#include <string>
#include <unordered_map>

#include "AssetTypes.hpp"

namespace Vanta {

    inline static std::unordered_map<std::string, AssetType> s_AssetExtensionMap =
    {
        { ".vscene", AssetType::Scene },
        { ".vmesh", AssetType::Mesh },
        { ".fbx", AssetType::MeshAsset },
        { ".gltf", AssetType::MeshAsset },
        { ".glb", AssetType::MeshAsset },
        { ".obj", AssetType::MeshAsset },
        { ".png", AssetType::Texture },
        { ".hdr", AssetType::EnvMap },
        { ".wav", AssetType::Audio },
        { ".ogg", AssetType::Audio }
    };

}