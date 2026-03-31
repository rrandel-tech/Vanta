set(ASSET_SOURCES
        AssetManager.cpp
        AssetSerializer.cpp
        AssetImporter.cpp
        MeshSerializer.cpp
)

set(ASSET_HEADERS
        AssetManager.hpp
        Asset.hpp
        AssetSerializer.hpp
        AssetImporter.hpp
        MeshSerializer.hpp
        AssetMetadata.hpp
        AssetTypes.hpp
)

target_sources(Vanta
        PRIVATE
        ${ASSET_SOURCES}
        ${ASSET_HEADERS}
)