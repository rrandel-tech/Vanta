#pragma once

#include "Asset.hpp"

#include <filesystem>

namespace Vanta {

    struct AssetMetadata
    {
        AssetHandle Handle = 0;
        AssetType Type;

        std::filesystem::path FilePath;
        bool IsDataLoaded = false;

        bool IsValid() const { return Handle != 0; }
    };
}