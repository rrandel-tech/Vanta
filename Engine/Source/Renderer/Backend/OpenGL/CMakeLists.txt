set(OPENGL_SOURCES
        OpenGLBuffer.cpp
        OpenGLFramebuffer.cpp
        OpenGLRendererAPI.cpp
        OpenGLShader.cpp
        OpenGLShaderUniform.cpp
        OpenGLTexture.cpp
)

set(OPENGL_HEADERS
        OpenGLBuffer.hpp
        OpenGLFramebuffer.hpp
        OpenGLShader.hpp
        OpenGLShaderUniform.hpp
        OpenGLTexture.hpp
)

target_sources(Vanta
        PRIVATE
        ${OPENGL_SOURCES}
        ${OPENGL_HEADERS}
)