#include "vapch.hpp"
#include "Window.hpp"

#include "Events/ApplicationEvent.hpp"
#include "Events/KeyEvent.hpp"
#include "Events/MouseEvent.hpp"

#include <glad/glad.h>

#include <imgui_impl_sdl3.h>

namespace Vanta {

    static void SDLErrorCallback(const char* context)
    {
        VA_CORE_ERROR("SDL Error ({}): {}", context, SDL_GetError());
    }

	Window* Window::Create(const WindowSpecification& specification)
    {
    	return new Window(specification);
    }

    Window::Window(const WindowSpecification& props)
        : m_specification(props), m_data()
    {
    }

    Window::~Window()
    {
        Window::Shutdown();
    }

    void Window::Init()
    {
        m_data.title = m_specification.Title;
        m_data.width = m_specification.Width;
        m_data.height = m_specification.Height;

        VA_CORE_INFO("Creating window {0} ({1}, {2})", m_specification.Title, m_specification.Width, m_specification.Height);

        SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#ifdef VA_DEBUG
    	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

    	Uint32 windowFlags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_OPENGL;

		switch (m_specification.Mode)
		{
			case WindowMode::Windowed:
				m_Window = SDL_CreateWindow(m_data.title.c_str(), m_data.width, m_data.height, windowFlags);
				break;
			case WindowMode::BorderlessFullscreen:
				windowFlags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN;
				m_Window = SDL_CreateWindow(m_data.title.c_str(), 0, 0, windowFlags);
				break;
			case WindowMode::ExclusiveFullscreen:
			{
				SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
				if (displayID == 0)
				{
					SDLErrorCallback("SDL_GetPrimaryDisplay");
					VA_CORE_ASSERT(false, "Could not get primary display!");
				}

				const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
				if (!mode)
				{
					SDLErrorCallback("SDL_GetCurrentDisplayMode");
					VA_CORE_ASSERT(false, "Could not get display mode!");
				}

				m_Window = SDL_CreateWindow(m_data.title.c_str(), mode->w, mode->h, windowFlags | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MOUSE_GRABBED);
				break;
			}
		}

		if (!m_Window)
		{
			SDLErrorCallback("SDL_CreateWindow");
			VA_CORE_ASSERT(false, "Could not create window!");
		}

    	m_GLContext = SDL_GL_CreateContext(m_Window);
    	if (!m_GLContext)
    	{
    		SDLErrorCallback("SDL_GL_CreateContext");
    		VA_CORE_ASSERT(false, "Could not create OpenGL context!");
    	}

    	SDL_GL_MakeCurrent(m_Window, m_GLContext);

    	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    	{
    		VA_CORE_ASSERT(false, "Failed to initialize GLAD!");
    	}

		// Update window size to actual size
		int w, h;
		SDL_GetWindowSize(m_Window, &w, &h);
		m_data.width = w;
		m_data.height = h;
	}

	void Window::PollEvents()
	{
    	if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendPlatformUserData != nullptr)
    	{
    		ImGui_ImplSDL3_ProcessEvent(&m_Event);
    	}

		SDL_WindowID windowID = SDL_GetWindowID(m_Window);
		if (windowID == 0)
		{
			SDLErrorCallback("SDL_GetWindowID");
			return;
		}

		while (SDL_PollEvent(&m_Event))
		{
			switch (m_Event.type)
			{
				case SDL_EVENT_QUIT:
				{
					WindowCloseEvent event;
					m_data.eventCallback(event);
					break;
				}
				case SDL_EVENT_WINDOW_RESIZED:
				{
					if (m_Event.window.windowID == windowID)
					{
						int width, height;
						SDL_GetWindowSize(m_Window, &width, &height);

						WindowResizeEvent event(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
						m_data.eventCallback(event);
						m_data.width = width;
						m_data.height = height;
					}
					break;
				}
				case SDL_EVENT_KEY_DOWN:
				{
					if (m_Event.key.windowID == windowID)
					{
						int repeatCount = m_Event.key.repeat ? 1 : 0;
						KeyPressedEvent event(m_Event.key.scancode, repeatCount);
						m_data.eventCallback(event);
					}
					break;
				}
				case SDL_EVENT_KEY_UP:
				{
					if (m_Event.key.windowID == windowID)
					{
						KeyReleasedEvent event(m_Event.key.scancode);
						m_data.eventCallback(event);
					}
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					if (m_Event.button.windowID == windowID)
					{
						MouseButtonPressedEvent event(m_Event.button.button);
						m_data.eventCallback(event);
					}
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					if (m_Event.button.windowID == windowID)
					{
						MouseButtonReleasedEvent event(m_Event.button.button);
						m_data.eventCallback(event);
					}
					break;
				}
				case SDL_EVENT_MOUSE_WHEEL:
				{
					if (m_Event.wheel.windowID == windowID)
					{
						MouseScrolledEvent event(m_Event.wheel.x, m_Event.wheel.y);
						m_data.eventCallback(event);
					}
					break;
				}
				case SDL_EVENT_MOUSE_MOTION:
				{
					if (m_Event.motion.windowID == windowID)
					{
						MouseMovedEvent event(m_Event.motion.x, m_Event.motion.y);
						m_data.eventCallback(event);
					}
					break;
				}
			}
		}
    }

    void Window::Shutdown()
    {
    	if (m_GLContext)
    	{
    		SDL_GL_DestroyContext(m_GLContext);
    		m_GLContext = nullptr;
    	}

    	if (m_Window)
    	{
    		SDL_DestroyWindow(m_Window);
    		m_Window = nullptr;
    	}
    	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    }

	std::pair<float, float> Window::GetWindowPos() const
    {
    	int x, y;
    	if (SDL_GetWindowPosition(m_Window, &x, &y) == false)
    	{
    		SDLErrorCallback("SDL_GetWindowPosition");
    		return { 0.0f, 0.0f };
    	}
    	return { static_cast<float>(x), static_cast<float>(y) };
    }

    void Window::ProcessEvents()
    {
    	PollEvents();
    }

    void Window::SwapBuffers()
    {
    	SDL_GL_SwapWindow(m_Window);
        // swapchain for vulkan
    }

	void Window::SetVSync(bool enabled)
    {
    	m_specification.VSync = enabled;

    	if (SDL_GL_SetSwapInterval(enabled ? 1 : 0) != 0)
    	{
    		SDLErrorCallback("SDL_GL_SetSwapInterval");
    	}

    	// swapchain for vulkan
    }

	bool Window::IsVSync() const
    {
    	return m_specification.VSync;
    }

	void Window::SetResizable(bool resizable) const
    {
    	if (SDL_SetWindowResizable(m_Window, resizable) == false)
    	{
    		SDLErrorCallback("SDL_SetWindowResizable");
    	}
    }

	void Window::Maximize()
    {
    	if (m_specification.Mode == WindowMode::Windowed)
    	{
    		if (SDL_MaximizeWindow(m_Window) == false)
    		{
    			SDLErrorCallback("SDL_MaximizeWindow");
    		}
    	}
    }

	void Window::CenterWindow()
    {
    	SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    	if (displayID == 0)
    	{
    		SDLErrorCallback("SDL_GetPrimaryDisplay");
    		return;
    	}

    	const SDL_DisplayMode* videomode = SDL_GetCurrentDisplayMode(displayID);
    	if (!videomode)
    	{
    		SDLErrorCallback("SDL_GetCurrentDisplayMode");
    		return;
    	}

    	int x = (videomode->w / 2) - (m_data.width / 2);
    	int y = (videomode->h / 2) - (m_data.height / 2);

    	if (SDL_SetWindowPosition(m_Window, x, y) == false)
    	{
    		SDLErrorCallback("SDL_SetWindowPosition");
    	}
    }

    void Window::SetTitle(const std::string& title)
    {
        m_data.title = title;
    	if (SDL_SetWindowTitle(m_Window, title.c_str()) == false)
    	{
    		SDLErrorCallback("SDL_SetWindowTitle");
    	}
    }

}