#include "vapch.hpp"
#include "OpenGLContext.hpp"

#include <glad/glad.h>

#include "Core/Log.hpp"

namespace Vanta {

	OpenGLContext::OpenGLContext()
	{
	}

	OpenGLContext::~OpenGLContext()
	{
	}

	void OpenGLContext::Init()
	{
		VA_CORE_INFO("OpenGLContext::Create");

		m_GLContext = SDL_GL_CreateContext(m_WindowHandle);
		VA_CORE_ASSERT(m_GLContext, "Failed to create OpenGL context!");

		SDL_GL_MakeCurrent(m_WindowHandle, m_GLContext);

		int status = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
		VA_CORE_ASSERT(status, "Failed to initialize Glad!");

		VA_CORE_INFO("OpenGL Info:");
		VA_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
		VA_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
		VA_CORE_INFO("  Version: {0}", (const char*)glGetString(GL_VERSION));

#ifdef VA_ENABLE_ASSERTS
		int versionMajor;
		int versionMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

		VA_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5), "Vanta requires at least OpenGL version 4.5!");
#endif
	}

}