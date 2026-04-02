#include "vapch.hpp"
#include "AssetRegistry.hpp"

#include "Project/Project.hpp"

namespace Vanta {

#define VA_ASSETREGISTRY_LOG 0
#if VA_ASSETREGISTRY_LOG
#define ASSET_LOG(...) VA_CORE_TRACE(__VA_ARGS__)
#else 
#define ASSET_LOG(...)
#endif

    static std::filesystem::path GetKey(const std::filesystem::path& path)
    {
        auto key = std::filesystem::relative(path, Project::GetAssetDirectory());
        if (key.empty())
            key = path.lexically_normal();
		
        return key;
    }

    AssetMetadata& AssetRegistry::operator[](const std::filesystem::path& path)
    {
        auto key = GetKey(path);
        ASSET_LOG("[ASSET] Retrieving key {0} (path = {1})", key.string(), path.string());
        VA_CORE_ASSERT(!path.string().empty());
        return m_AssetRegistry[key];
    }

    const AssetMetadata& AssetRegistry::Get(const std::filesystem::path& path) const
    {
        auto key = GetKey(path);
        VA_CORE_ASSERT(m_AssetRegistry.find(key) != m_AssetRegistry.end());
        ASSET_LOG("[ASSET] Retrieving const {0} (path = {1})", key.string(), path.string());
        VA_CORE_ASSERT(!path.string().empty());
        return m_AssetRegistry.at(key);
    }

    bool AssetRegistry::Contains(const std::filesystem::path& path) const
    {
        auto key = GetKey(path);
        ASSET_LOG("[ASSET] Contains key {0} (path = {1})", key.string(), path.string());
        return m_AssetRegistry.find(key) != m_AssetRegistry.end();
    }

    size_t AssetRegistry::Remove(const std::filesystem::path& path)
    {
        auto key = GetKey(path);
        ASSET_LOG("[ASSET] Removing key {0} (path = {1})", key.string(), path.string());
        return m_AssetRegistry.erase(key);
    }

    void AssetRegistry::Clear()
    {
        ASSET_LOG("[ASSET] Clearing registry");
        m_AssetRegistry.clear();
    }

}