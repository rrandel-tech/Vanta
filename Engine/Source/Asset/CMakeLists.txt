set(ASSET_SOURCES
        AssetManager.cpp
        AssetSerializer.cpp
        AssetImporter.cpp
        MeshSerializer.cpp
        AssetRegistry.cpp
)

set(ASSET_HEADERS
        AssetManager.hpp
        Asset.hpp
        AssetExtensions.hpp
        AssetSerializer.hpp
        AssetImporter.hpp
        MeshSerializer.hpp
        AssetMetadata.hpp
        AssetTypes.hpp
        AssetRegistry.hpp
)

target_sources(Vanta
        PRIVATE
        ${ASSET_SOURCES}
        ${ASSET_HEADERS}
)