set(EDITOR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

set(EDITOR_SOURCES
        ${EDITOR_SRC_DIR}/Vanta-Editor.cpp
        ${EDITOR_SRC_DIR}/EditorLayer.cpp
)

set(EDITOR_HEADERS
        ${EDITOR_SRC_DIR}/EditorLayer.hpp
)

add_executable(Vanta-Editor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

target_compile_features(Vanta-Editor PRIVATE cxx_std_23)
target_include_directories(Vanta-Editor PUBLIC ${EDITOR_SRC_DIR})

target_link_libraries(Vanta-Editor PRIVATE Vanta)

vanta_enable_lto(Vanta-Editor)
vanta_copy_directory(Vanta-Editor Editor/Resources Resources)
vanta_copy_directory(Vanta-Editor Editor/SandboxProject SandboxProject)