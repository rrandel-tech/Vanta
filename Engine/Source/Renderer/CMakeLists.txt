set(RENDERER_SOURCES
        RenderCommandQueue.cpp
        Renderer.cpp
)

set(RENDERER_HEADERS
        RenderCommandQueue.hpp
        Renderer.hpp
        RendererAPI.hpp
)

target_sources(Vanta
        PRIVATE
        ${RENDERER_SOURCES}
        ${RENDERER_HEADERS}
)

add_subdirectory(Backend/OpenGL)