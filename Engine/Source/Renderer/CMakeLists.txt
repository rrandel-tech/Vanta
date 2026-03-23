set(RENDERER_SOURCES
        RenderCommandQueue.cpp
        Renderer.cpp
        Buffer.cpp
        Shader.cpp
)

set(RENDERER_HEADERS
        RenderCommandQueue.hpp
        Renderer.hpp
        RendererAPI.hpp
        Buffer.hpp
        Shader.hpp
)

target_sources(Vanta
        PRIVATE
        ${RENDERER_SOURCES}
        ${RENDERER_HEADERS}
)

add_subdirectory(Backend/OpenGL)