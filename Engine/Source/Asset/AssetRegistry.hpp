#pragma once

#include <unordered_map>
#include <filesystem>

#include "AssetMetadata.hpp"

/* namespace std {

    template <>
    struct hash<std::filesystem::path>
    {
        std::size_t operator()(const std::filesystem::path& path) const
        {
            return hash_value(path);
        }
    };

} */

namespace Vanta {

    class AssetRegistry
    {
    public:
        AssetMetadata& operator[](const std::filesystem::path& path);
        const AssetMetadata& Get(const std::filesystem::path& path) const;

        bool Contains(const std::filesystem::path& path) const;
        size_t Remove(const std::filesystem::path& path);
        void Clear();

        std::unordered_map<std::filesystem::path, AssetMetadata>::iterator begin() { return m_AssetRegistry.begin(); }
        std::unordered_map<std::filesystem::path, AssetMetadata>::iterator end() { return m_AssetRegistry.end(); }
        std::unordered_map<std::filesystem::path, AssetMetadata>::const_iterator cbegin() { return m_AssetRegistry.cbegin(); }
        std::unordered_map<std::filesystem::path, AssetMetadata>::const_iterator cend() { return m_AssetRegistry.cend(); }
    private:
        std::unordered_map<std::filesystem::path, AssetMetadata> m_AssetRegistry;
    };

}