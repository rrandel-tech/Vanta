#pragma once

#include "Renderer/RendererContext.hpp"
#include <SDL3/SDL.h>

struct SDL_Window;

namespace Vanta {

	class OpenGLContext : public RendererContext
	{
	public:
		OpenGLContext();
		virtual ~OpenGLContext();

		virtual void Init() override;
	private:
		SDL_Window* m_WindowHandle;
		SDL_GLContext m_GLContext = nullptr;
	};

}