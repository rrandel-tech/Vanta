#include "vapch.hpp"
#include "Window.hpp"

namespace Vanta {

    std::unique_ptr<Window> Window::Create(const WindowSpecification& specification)
    {
        return std::make_unique<Window>(specification);
    }

    Window::Window(const WindowSpecification& props)
        : m_specification(props), m_data()
    {
    }

    Window::~Window()
    {
        Window::shutdown();
    }

    void Window::Init()
    {
        m_data.title = m_specification.title;
        m_data.width = m_specification.width;
        m_data.height = m_specification.height;

        VA_CORE_INFO("Creating window {0} ({1}, {2})", m_specification.title, m_specification.width, m_specification.height);
    }

    void Window::shutdown()
    {
    }

    void Window::ProcessEvents()
    {
    }

    void Window::SwapBuffers()
    {
        // swapchain
    }

    void Window::SetTitle(const std::string& title)
    {
        m_data.title = title;
    }

}