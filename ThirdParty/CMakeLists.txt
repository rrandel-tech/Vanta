# ==== spdlog ====
set(SPDLOG_USE_STD_FORMAT ON)
set(SPDLOG_ENABLE_PCH ON)
set(SPDLOG_MSVC_UTF8 ON)
set(SPDLOG_BUILD_EXAMPLE OFF)
set(SPDLOG_BUILD_TESTS OFF)
set(SPDLOG_BUILD_BENCH OFF)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/spdlog SYSTEM)

# ==== SDL3 ====
set(SDL_AUDIO OFF)
set(SDL_EXAMPLES OFF)
set(SDL_SAMPLES OFF)
set(SDL_TESTS OFF)
set(SDL_STATIC ON)
set(SDL_VULKAN ON)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SDL3 SYSTEM)

# ==== ImGui ====
set(IMGUI_DIR ${CMAKE_CURRENT_LIST_DIR}/imgui)

add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp

        # Backends
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
)

target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC SDL3::SDL3-static glad)

target_compile_definitions(imgui PUBLIC IMGUI_DEFINE_MATH_OPERATORS IMGUI_IMPL_OPENGL_LOADER_GLAD)

# ==== glad (OpenGL loader) ====
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/glad SYSTEM)

# ==== GLM (Header-only library) ====
set(GLM_BUILD_TESTS OFF)
set(GLM_BUILD_INSTALL OFF)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/glm SYSTEM)

target_compile_definitions(glm PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

# ==== Assimp (3D Model Loader) ====
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_NO_EXPORT ON)
set(BUILD_SHARED_LIBS OFF)

set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)
set(ASSIMP_BUILD_FBX_IMPORTER ON)

set(ASSIMP_BUILD_DRACO OFF)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/assimp SYSTEM)

# ==== stb (Single header libraries) ====
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_LIST_DIR}/stb)
add_library(stb::stb ALIAS stb)

# ==== EnTT (Header-only ECS library) ====
add_library(entt INTERFACE)
target_include_directories(entt INTERFACE ${CMAKE_CURRENT_LIST_DIR}/entt/include)
add_library(entt::entt ALIAS entt)

# ==== FastNoise (Header-only noise library) ====
add_library(fastnoise INTERFACE)
target_include_directories(fastnoise INTERFACE ${CMAKE_CURRENT_LIST_DIR}/FastNoise)
add_library(fastnoise::fastnoise ALIAS fastnoise)

# ==== yaml-cpp ====
set(YAML_CPP_BUILD_TESTS OFF)
set(YAML_CPP_BUILD_TOOLS OFF)
set(YAML_CPP_BUILD_CONTRIB OFF)
set(YAML_CPP_INSTALL OFF)
set(YAML_BUILD_SHARED_LIBS OFF)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/yaml-cpp SYSTEM)

# ==== Lua 5.4.8 ====
set(LUA_DIR ${CMAKE_CURRENT_LIST_DIR}/lua)

add_library(lua STATIC
        ${LUA_DIR}/lapi.c
        ${LUA_DIR}/lauxlib.c
        ${LUA_DIR}/lbaselib.c
        ${LUA_DIR}/lcode.c
        ${LUA_DIR}/lcorolib.c
        ${LUA_DIR}/lctype.c
        ${LUA_DIR}/ldblib.c
        ${LUA_DIR}/ldebug.c
        ${LUA_DIR}/ldo.c
        ${LUA_DIR}/ldump.c
        ${LUA_DIR}/lfunc.c
        ${LUA_DIR}/lgc.c
        ${LUA_DIR}/linit.c
        ${LUA_DIR}/liolib.c
        ${LUA_DIR}/llex.c
        ${LUA_DIR}/lmathlib.c
        ${LUA_DIR}/lmem.c
        ${LUA_DIR}/loadlib.c
        ${LUA_DIR}/lobject.c
        ${LUA_DIR}/lopcodes.c
        ${LUA_DIR}/loslib.c
        ${LUA_DIR}/lparser.c
        ${LUA_DIR}/lstate.c
        ${LUA_DIR}/lstring.c
        ${LUA_DIR}/lstrlib.c
        ${LUA_DIR}/ltable.c
        ${LUA_DIR}/ltablib.c
        ${LUA_DIR}/ltm.c
        ${LUA_DIR}/lundump.c
        ${LUA_DIR}/lutf8lib.c
        ${LUA_DIR}/lvm.c
        ${LUA_DIR}/lzio.c
)

target_include_directories(lua PUBLIC ${LUA_DIR})

# ==== sol2 (Header-only Lua binding) ====
add_library(sol2 INTERFACE)

target_include_directories(sol2 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/sol2/include
)

add_library(sol2::sol2 ALIAS sol2)