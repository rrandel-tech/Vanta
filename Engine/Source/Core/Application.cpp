#include "vapch.hpp"
#include "Application.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/Framebuffer.hpp"

#include <imgui.h>
#include <glad/glad.h>

namespace Vanta {

    Application* Application::s_Instance = nullptr;

    Application::Application(const ApplicationSpecification& specification)
        : m_Specification(specification)
    {
        s_Instance = this;

        WindowSpecification windowSpec;
        windowSpec.Title = m_Specification.Name;
        windowSpec.Width = m_Specification.WindowWidth;
        windowSpec.Height = m_Specification.WindowHeight;
        m_Window = std::unique_ptr<Window>(Window::Create(windowSpec));
        m_Window->Init();
        m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });

        m_Window->SetResizable(m_Specification.Resizable);
        if (windowSpec.Mode == WindowMode::Windowed)
            m_Window->CenterWindow();

        m_ImGuiLayer = new ImGuiLayer("ImGui");
        PushOverlay(m_ImGuiLayer);

        Renderer::Init();
        Renderer::Get().WaitAndRender();
    }

    Application::~Application()
    {
        m_Window->SetEventCallback([](Event& e) {});

        for (Layer* layer : m_LayerStack)
        {
            layer->OnDetach();
            delete layer;
        }
    }

    void Application::PushLayer(Layer* layer)
    {
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer* layer)
    {
        m_LayerStack.PushOverlay(layer);
        layer->OnAttach();
    }

    void Application::PopLayer(Layer* layer)
    {
        m_LayerStack.PopLayer(layer);
        layer->OnDetach();
    }

    void Application::PopOverlay(Layer* layer)
    {
        m_LayerStack.PopOverlay(layer);
        layer->OnDetach();
    }

    void Application::RenderImGui()
    {
        m_ImGuiLayer->Begin();

        ImGui::Begin("Renderer");
        auto& caps = RendererAPI::GetCapabilities();
        ImGui::Text("Vendor: %s", caps.Vendor.c_str());
        ImGui::Text("Renderer: %s", caps.Renderer.c_str());
        ImGui::Text("Version: %s", caps.Version.c_str());
        ImGui::Text("Frame Time: %.2fms\n", m_TimeStep.GetMilliseconds());
        ImGui::End();

        for (Layer* layer : m_LayerStack)
            layer->OnImGuiRender();

        m_ImGuiLayer->End();
    }

    void Application::Run()
    {
        OnInit();
        while (m_Running)
        {
            processEvents();

            if (!m_Minimized)
            {
                for (Layer* layer : m_LayerStack)
                    layer->OnUpdate(m_TimeStep);

                // Render ImGui on render thread
                Application* app = this;
                VA_RENDER_1(app, { app->RenderImGui(); });

                Renderer::Get().WaitAndRender();

                m_Window->SwapBuffers();
            }

            Timestep m_frametime;
            m_frametime = GetTime();
            m_TimeStep = (m_frametime < Timestep(0.0333f)) ? m_frametime : Timestep(0.0333f);
            m_LastFrameTime += m_frametime; // Keep total time
            VA_CORE_TRACE("Timestep: {:.3f}ms ({:.1f} FPS)", m_TimeStep * 1000.0f, 1.0f / m_TimeStep);
        }
        OnShutdown();
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::OnShutdown()
    {
    }

    void Application::processEvents()
    {
        m_Window->ProcessEvents();
    }

    void Application::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
        dispatcher.Dispatch<WindowMinimizeEvent>([this](WindowMinimizeEvent& e) { return OnWindowMinimize(e); });
        dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
        {
            (*--it)->OnEvent(event);
            if (event.Handled)
                break;
        }

        if (event.Handled)
            return;

        // TODO: Should these callbacks be called BEFORE the layers recieve events?
        //				We may actually want that since most of these callbacks will be functions REQUIRED in order for the game
        //				to work, and if a layer has already handled the event we may end up with problems
        for (auto& eventCallback : m_EventCallbacks)
        {
            eventCallback(event);

            if (event.Handled)
                break;
        }
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        int width = e.GetWidth(), height = e.GetHeight();
        if (width == 0 || height == 0)
        {
            m_Minimized = true;
            return false;
        }
        m_Minimized = false;
        VA_RENDER_2(width, height, { glViewport(0, 0, width, height); });
        auto& fbs = FramebufferPool::GetGlobal()->GetAll();
        for (auto& fb : fbs)
            fb->Resize(width, height);
        return false;
    }

    bool Application::OnWindowMinimize(WindowMinimizeEvent& e)
    {
        m_Minimized = e.IsMinimized();
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

    float Application::GetTime() const
    {
        static uint64_t last = SDL_GetTicksNS();
        uint64_t now = SDL_GetTicksNS();
        uint64_t delta = now - last;
        last = now;
        return static_cast<float>(delta * 1e-9f);
    }

    std::string Application::OpenFile(const std::string& filter) const
    {
        OPENFILENAMEA ofn;       // common dialog box structure
        CHAR szFile[260] = { 0 };       // if using TCHAR macros

        // Initialize OPENFILENAME
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(static_cast<SDL_Window*>(m_Window->GetNativeWindow())), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter.c_str();
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            return ofn.lpstrFile;
        }
        return std::string();
    }

}
