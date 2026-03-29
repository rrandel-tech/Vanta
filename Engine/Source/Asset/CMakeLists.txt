set(ASSET_SOURCES
        AssetManager.cpp
        AssetSerializer.cpp
)

set(ASSET_HEADERS
        AssetManager.hpp
        Asset.hpp
        AssetSerializer.hpp
)

target_sources(Vanta
        PRIVATE
        ${ASSET_SOURCES}
        ${ASSET_HEADERS}
)