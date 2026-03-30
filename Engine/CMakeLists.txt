set(VANTA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# ==== Root-level infrastructure files ====
set(VANTA_ROOT_SOURCES
        ${VANTA_SRC_DIR}/vapch.cpp
)

set(VANTA_ROOT_HEADERS
        ${VANTA_SRC_DIR}/Vanta.hpp
        ${VANTA_SRC_DIR}/EntryPoint.hpp
)

# ==== Third-party implementation files ====
set(VANTA_THIRDPARTY_IMPL_SOURCES
        ${VANTA_SRC_DIR}/ThirdParty/stb_impl.cpp
)

add_library(Vanta STATIC)

target_sources(Vanta PRIVATE ${VANTA_ROOT_SOURCES} ${VANTA_ROOT_HEADERS} ${VANTA_THIRDPARTY_IMPL_SOURCES})

add_subdirectory(${VANTA_SRC_DIR}/Asset)
add_subdirectory(${VANTA_SRC_DIR}/Core)
add_subdirectory(${VANTA_SRC_DIR}/Editor)
add_subdirectory(${VANTA_SRC_DIR}/Events)
add_subdirectory(${VANTA_SRC_DIR}/ImGui)
add_subdirectory(${VANTA_SRC_DIR}/Math)
add_subdirectory(${VANTA_SRC_DIR}/Platform)
add_subdirectory(${VANTA_SRC_DIR}/Renderer)
add_subdirectory(${VANTA_SRC_DIR}/Scene)
add_subdirectory(${VANTA_SRC_DIR}/Utilities)

target_precompile_headers(Vanta PRIVATE ${VANTA_SRC_DIR}/vapch.hpp)

target_compile_features(Vanta PUBLIC cxx_std_23)
target_include_directories(Vanta PUBLIC ${VANTA_SRC_DIR})

target_compile_definitions(Vanta PUBLIC
        NOMINMAX
        SDL_MAIN_HANDLED
)

target_link_libraries(Vanta
        PUBLIC

        # ==== Core Dependencies ====
        spdlog
        SDL3::SDL3-static
        imgui
        glad
        glm
        assimp::assimp
        stb::stb
        entt::entt
        fastnoise::fastnoise
        yaml-cpp::yaml-cpp

        lua
        sol2::sol2

        Vulkan::Vulkan
        shaderc::shaderc
        spirv-cross::spirv-cross
        spirv-tools::spirv-tools
)

vanta_enable_lto(Vanta)