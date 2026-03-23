set(OPENGL_SOURCES
        OpenGLRendererAPI.cpp
)

target_sources(Vanta
        PRIVATE
        ${OPENGL_SOURCES}
)