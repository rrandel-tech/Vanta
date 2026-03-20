set(EDITOR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

set(EDITOR_SOURCES
        ${EDITOR_SRC_DIR}/Vanta-Editor.cpp
)

add_executable(Vanta-Editor ${EDITOR_SOURCES})

target_compile_features(Vanta-Editor PRIVATE cxx_std_23)
target_include_directories(Vanta-Editor PUBLIC ${EDITOR_SRC_DIR})

target_link_libraries(Vanta-Editor PRIVATE Vanta)

vanta_enable_lto(Vanta-Editor)