#pragma once

#include "Core/UUID.hpp"

#include <entt/entt.hpp>

namespace Vanta {

    enum class AssetType : int8_t
    {
        Scene,
        Mesh,
        Texture,
        EnvMap,
        Audio,
        Script,
        Directory,
        Other,
        None,
        Missing
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

        virtual bool operator==(const Asset& other) const
        {
            return Handle == other.Handle;
        }

        virtual bool operator!=(const Asset& other) const
        {
            return !(*this == other);
        }
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