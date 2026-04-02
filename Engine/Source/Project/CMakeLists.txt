set(PROJECT_SOURCES
        Project.cpp
        ProjectSerializer.cpp
)

set(PROJECT_HEADERS
        Project.hpp
        ProjectSerializer.hpp
)

target_sources(Vanta
        PRIVATE
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
)