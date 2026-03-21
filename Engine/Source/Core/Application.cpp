#include "vapch.hpp"
#include "Application.hpp"

namespace Vanta {

    Application::Application(const ApplicationSpecification& specification)
        : m_specification(specification)
    {
        WindowSpecification windowSpec;
        windowSpec.title = m_specification.name;
        windowSpec.width = m_specification.windowWidth;
        windowSpec.height = m_specification.windowHeight;
        m_Window = std::unique_ptr(Window::Create(windowSpec));
        m_Window->Init();
    }

    Application::~Application()
    {
    }

    void Application::Close()
    {
        m_isRunning = false;
    }

    void Application::OnShutdown()
    {
    }

    void Application::Run()
    {
        OnInit();
        while (m_isRunning)
        {
            if (!m_isMinimized)
            {
            }
        }
        OnShutdown();
    }

    void Application::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
        dispatcher.Dispatch<WindowMinimizeEvent>([this](WindowMinimizeEvent& e) { return OnWindowMinimize(e); });
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });

        VA_CORE_INFO("{}", event.ToString());
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        return false;
    }

    bool Application::OnWindowMinimize(WindowMinimizeEvent& e)
    {
        m_isMinimized = e.IsMinimized();
        return false;
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        Close();
        return false; // give other things a chance to react to window close
    }

    const char* Application::GetConfigurationName()
    {
        return VA_BUILD_CONFIG_NAME;
    }

    const char* Application::GetPlatformName()
    {
        return VA_BUILD_PLATFORM_NAME;
    }

}