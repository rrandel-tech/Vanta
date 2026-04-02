set(UTILITIES_SOURCES
        StringUtils.cpp
        FileSystem.cpp
)

set(UTILITIES_HEADERS
        FileSystem.hpp
        StringUtils.hpp
)

target_sources(Vanta
        PRIVATE
        ${UTILITIES_SOURCES}
        ${UTILITIES_HEADERS}
)