set(VANTA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# ==== Root-level infrastructure files ====
set(VANTA_ROOT_SOURCES
        ${VANTA_SRC_DIR}/vapch.cpp
)

set(VANTA_ROOT_HEADERS
        ${VANTA_SRC_DIR}/Vanta.hpp
        ${VANTA_SRC_DIR}/EntryPoint.hpp
)

add_library(Vanta STATIC)

target_sources(Vanta PRIVATE ${VANTA_ROOT_SOURCES} ${VANTA_ROOT_HEADERS})

add_subdirectory(${VANTA_SRC_DIR}/Core)

target_precompile_headers(Vanta PRIVATE ${VANTA_SRC_DIR}/vapch.hpp)

target_compile_features(Vanta PUBLIC cxx_std_23)
target_include_directories(Vanta PUBLIC ${VANTA_SRC_DIR})

target_link_libraries(Vanta
        PUBLIC

        # ==== Core Dependencies ====
        spdlog
)

vanta_enable_lto(Vanta)