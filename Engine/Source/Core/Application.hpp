#pragma once

#include "Base.hpp"
#include "TimeStep.hpp"
#include "Window.hpp"
#include "LayerStack.hpp"

#include "Events/ApplicationEvent.hpp"

#include "ImGui/ImGuiLayer.hpp"

#include <string>
#include <memory>

namespace Vanta {

    struct ApplicationSpecification
    {
        std::string Name = "Vanta";
        uint32_t WindowWidth = 1600, WindowHeight = 900;
        WindowMode Mode = WindowMode::Windowed;
        bool VSync = true;
        bool Resizable = true;
    };

    class Application
    {
        using EventCallbackFn = std::function<void(Event&)>;
    public:
        explicit Application(const ApplicationSpecification& specification);
        virtual ~Application();

        void Run();
        void Close();

        virtual void OnInit() {}

        void OnShutdown();

        void OnUpdate(Timestep ts) {}
        void OnEvent(Event& event);

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* layer);
        void PopLayer(Layer* layer);
        void PopOverlay(Layer* layer);
        void RenderImGui();

        void AddEventCallback(const EventCallbackFn& eventCallback) { m_EventCallbacks.push_back(eventCallback); }

        inline Window& GetWindow() { return *m_Window; }

        static inline Application& Get() { return *s_Instance; }

        float GetTime() const; // TODO: This should be in "Platform"

        static const char* GetConfigurationName();
        static const char* GetPlatformName();

        [[nodiscard]] const ApplicationSpecification& GetSpecification() const { return m_Specification; }

        std::string OpenFile(const std::string& filter) const;
    private:
        void processEvents();

        bool OnWindowResize(WindowResizeEvent& e);
        bool OnWindowMinimize(WindowMinimizeEvent& e);
        bool OnWindowClose(WindowCloseEvent& e);
    private:
        std::unique_ptr<Window> m_Window;
        ApplicationSpecification m_Specification;
        bool m_Running = true, m_Minimized = false;
        LayerStack m_LayerStack;
        ImGuiLayer* m_ImGuiLayer;
        Timestep m_TimeStep;

        float m_LastFrameTime = 0.0f;

        std::vector<EventCallbackFn> m_EventCallbacks;

        static Application* s_Instance;
    };

    // Implemented by CLIENT
    Application* CreateApplication(int argc, char** argv);
}