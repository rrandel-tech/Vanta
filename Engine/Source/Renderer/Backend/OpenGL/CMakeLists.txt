set(OPENGL_SOURCES
		OpenGLFramebuffer.cpp
		OpenGLIndexBuffer.cpp
		OpenGLPipeline.cpp
		OpenGLRendererAPI.cpp
		OpenGLRenderPass.cpp
		OpenGLShader.cpp
		OpenGLShaderUniform.cpp
		OpenGLTexture.cpp
		OpenGLVertexBuffer.cpp
)

set(OPENGL_HEADERS
		OpenGLFramebuffer.hpp
		OpenGLIndexBuffer.hpp
		OpenGLPipeline.hpp
		OpenGLRenderPass.hpp
		OpenGLShader.hpp
		OpenGLShaderUniform.hpp
		OpenGLTexture.hpp
		OpenGLVertexBuffer.hpp
)

target_sources(Vanta
		PRIVATE
		${OPENGL_SOURCES}
		${OPENGL_HEADERS}
)