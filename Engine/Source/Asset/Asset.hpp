#pragma once

#include "Core/UUID.hpp"

#include <entt/entt.hpp>

#include <string>

namespace Vanta {

    enum class AssetType
    {
        Scene,
        Mesh,
        Texture,
        EnvMap,
        Audio,
        Script,
        Directory,
        Other,
        None
    };

    using AssetHandle = UUID;

    class Asset : public RefCounted
    {
    public:
        AssetHandle Handle;
        AssetType Type = AssetType::None;

        std::string FilePath;
        std::string FileName;
        std::string Extension;
        AssetHandle ParentDirectory;
        bool IsDataLoaded = false;

        virtual ~Asset() {}
    };

    // Treating directories as assets simplifies the asset manager window rendering by a lot
    class Directory : public Asset
    {
    public:
        std::vector<AssetHandle> ChildDirectories;

        Directory() = default;
    };
}