#include "vapch.hpp"
#include "Application.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/Framebuffer.hpp"

#include "Asset/AssetManager.hpp"

#include "Debug/Profiler.hpp"

#include "Renderer/Backend/Vulkan/VulkanRenderer.hpp"
#include "Renderer/Backend/Vulkan/VulkanAllocator.hpp"
#include "Renderer/Backend/Vulkan/VulkanSwapChain.hpp"

#include <imgui.h>
#include "imgui_internal.h"
#include <glad/glad.h>

#include <filesystem>

extern bool g_ApplicationRunning;
extern ImGuiContext* GImGui;

namespace Vanta {

    Application* Application::s_Instance = nullptr;

    Application::Application(const ApplicationSpecification& specification)
        : m_Specification(specification)
    {
        s_Instance = this;

        if (!specification.WorkingDirectory.empty())
            std::filesystem::current_path(specification.WorkingDirectory);

        m_Profiler = new PerformanceProfiler();

        WindowSpecification windowSpec;
        windowSpec.Title = m_Specification.Name;
        windowSpec.Width = m_Specification.WindowWidth;
        windowSpec.Height = m_Specification.WindowHeight;
        windowSpec.VSync = m_Specification.VSync;
        m_Window = std::unique_ptr<Window>(Window::Create(windowSpec));
        m_Window->Init();
        m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });
        m_Window->SetVSync(false);

        m_Window->SetResizable(m_Specification.Resizable);
        if (windowSpec.Mode == WindowMode::Windowed)
            m_Window->CenterWindow();

        // Init renderer and execute command queue to compile all shaders
        Renderer::Init();
        Renderer::WaitAndRender();

        if (m_Specification.EnableImGui)
        {
            m_ImGuiLayer = ImGuiLayer::Create();
            PushOverlay(m_ImGuiLayer);
        }
    }

    Application::~Application()
    {
        m_Window->SetEventCallback([](Event& e) {});

        for (Layer* layer : m_LayerStack)
        {
            layer->OnDetach();
            delete layer;
        }

        FramebufferPool::GetGlobal()->GetAll().clear();

        AssetManager::Shutdown();

        Renderer::WaitAndRender();

        for (uint32_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++)
        {
            auto& queue = Renderer::GetRenderResourceReleaseQueue(i);
            queue.Execute();
        }

        Renderer::Shutdown();

        delete m_Profiler;
        m_Profiler = nullptr;
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
        VA_SCOPE_PERF("Application::RenderImGui");
        VA_PROFILE_FUNC();

        m_ImGuiLayer->Begin();
        ImGui::Begin("Renderer");
        auto& caps = Renderer::GetCapabilities();
        ImGui::Text("Vendor: %s", caps.Vendor.c_str());
        ImGui::Text("Renderer: %s", caps.Device.c_str());
        ImGui::Text("Version: %s", caps.Version.c_str());
        ImGui::Separator();
        ImGui::Text("Frame Time: %.2fms\n", m_TimeStep.GetMilliseconds());

        if (RendererAPI::Current() == RendererAPIType::Vulkan)
        {
            GPUMemoryStats memoryStats = VulkanAllocator::GetStats();
            std::string used = Utils::BytesToString(memoryStats.Used);
            std::string free = Utils::BytesToString(memoryStats.Free);
            ImGui::Text("Used VRAM: %s", used.c_str());
            ImGui::Text("Free VRAM: %s", free.c_str());
        }

        bool vsync = m_Window->IsVSync();
        if (ImGui::Checkbox("Vsync", &vsync))
            m_Window->SetVSync(vsync);
        ImGui::End();

        ImGui::Begin("Performance");
        ImGui::Text("Frame Time: %.2fms\n", m_TimeStep.GetMilliseconds());
        const auto& perFrameData = m_Profiler->GetPerFrameData();
        for (auto&& [name, time] : perFrameData)
        {
            ImGui::Text("%s: %.3fms\n", name, time);
        }

        ImGui::End();
        m_Profiler->Clear();

        for (Layer* layer : m_LayerStack)
            layer->OnImGuiRender();
    }

    void Application::Run()
    {
        OnInit();
        while (m_Running)
        {
            static uint64_t frameCounter = 0;

            ProcessEvents();

            if (!m_Minimized)
            {
                Renderer::BeginFrame();

                {
                    VA_SCOPE_PERF("Application Layer::OnUpdate");
                    for (Layer* layer : m_LayerStack)
                        layer->OnUpdate(m_TimeStep);
                }

                // Render ImGui on render thread
                Application* app = this;
                if (m_Specification.EnableImGui)
                {
                    Renderer::Submit([app]() { app->RenderImGui(); });
                    Renderer::Submit([=]() {m_ImGuiLayer->End(); });
                }
                Renderer::EndFrame();

                // On Render thread
                m_Window->GetSwapChain().BeginFrame();
                Renderer::WaitAndRender();

                m_Window->SwapBuffers();
            }

            Timestep m_frametime;
            m_frametime = GetTime();
            m_TimeStep = (m_frametime < Timestep(0.0333f)) ? m_frametime : Timestep(0.0333f);
            m_LastFrameTime += m_frametime; // Keep total time
            // VA_CORE_TRACE("Timestep: {:.3f}ms ({:.1f} FPS)", m_TimeStep * 1000.0f, 1.0f / m_TimeStep);

            frameCounter++;
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

    void Application::ProcessEvents()
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

        m_Window->GetSwapChain().OnResize(width, height);

#if 0
        auto& fbs = FramebufferPool::GetGlobal()->GetAll();
        for (auto& fb : fbs)
        {
            if (!fb->GetSpecification().NoResize)
                fb->Resize(width, height);
        }
#endif

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

    std::string Application::OpenFile(const char* filter) const
    {
        OPENFILENAMEA ofn;       // common dialog box structure
        CHAR szFile[260] = { 0 };       // if using TCHAR macros

        // Initialize OPENFILENAME
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(static_cast<SDL_Window*>(m_Window->GetNativeWindow())), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            return ofn.lpstrFile;
        }
        return std::string();
    }

    std::string Application::SaveFile(const char* filter) const
    {
        OPENFILENAMEA ofn;       // common dialog box structure
        CHAR szFile[260] = { 0 };       // if using TCHAR macros

        // Initialize OPENFILENAME
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(static_cast<SDL_Window*>(m_Window->GetNativeWindow())), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&ofn) == TRUE)
        {
            return ofn.lpstrFile;
        }
        return std::string();
    }

}
