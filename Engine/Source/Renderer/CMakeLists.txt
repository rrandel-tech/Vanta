set(RENDERER_SOURCES
        Buffer.cpp
        Camera.cpp
        Framebuffer.cpp
        Mesh.cpp
        Material.cpp
        RenderCommandQueue.cpp
        Renderer.cpp
        Shader.cpp
        Texture.cpp
        VertexArray.cpp
)

set(RENDERER_HEADERS
        Buffer.hpp
        Camera.hpp
        Framebuffer.hpp
        Mesh.hpp
        Material.hpp
        RenderCommandQueue.hpp
        Renderer.hpp
        RendererAPI.hpp
        Shader.hpp
        ShaderUniform.hpp
        Texture.hpp
        VertexArray.hpp
)

target_sources(Vanta
        PRIVATE
        ${RENDERER_SOURCES}
        ${RENDERER_HEADERS}
)

add_subdirectory(Backend/OpenGL)