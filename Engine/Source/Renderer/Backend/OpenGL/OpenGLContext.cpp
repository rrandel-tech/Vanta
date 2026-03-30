#include "vapch.hpp"
#include "OpenGLContext.hpp"

#include <glad/glad.h>

#include "Core/Log.hpp"

namespace Vanta {

	OpenGLContext::OpenGLContext(SDL_Window* windowHandle)
		: m_WindowHandle(windowHandle), m_GLContext(nullptr)
	{
	}

	OpenGLContext::~OpenGLContext()
	{
		if (m_GLContext)
			SDL_GL_DestroyContext(m_GLContext);
	}

	void OpenGLContext::Create()
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

	void OpenGLContext::SwapBuffers()
	{
		SDL_GL_SwapWindow(m_WindowHandle);
	}

}