set(OPENGL_SOURCES
        OpenGLRendererAPI.cpp
        OpenGLBuffer.cpp
        OpenGLShader.cpp
)

set(OPENGL_HEADERS
        OpenGLBuffer.hpp
        OpenGLShader.hpp
)

target_sources(Vanta
        PRIVATE
        ${OPENGL_SOURCES}
        ${OPENGL_HEADERS}
)