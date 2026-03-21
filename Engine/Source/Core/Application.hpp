#pragma once

#include "Core/Base.hpp"
#include "Window.hpp"

#include "Events/ApplicationEvent.hpp"

#include <string>
#include <memory>

namespace Vanta {

    struct ApplicationSpecification
    {
        std::string name = "Vanta";
        uint32_t windowWidth = 1600, windowHeight = 900;
    };

    class Application
    {
    public:
        explicit Application(const ApplicationSpecification& specification);
        virtual ~Application();

        void Run();
        void Close();

        virtual void OnInit() {}

        void OnShutdown();

        void OnUpdate() {}
        void OnEvent(Event& event);

        static const char* GetConfigurationName();
        static const char* GetPlatformName();

        [[nodiscard]] const ApplicationSpecification& GetSpecification() const { return m_specification; }
    private:
        bool OnWindowResize(WindowResizeEvent& e);
        bool OnWindowMinimize(WindowMinimizeEvent& e);
        bool OnWindowClose(WindowCloseEvent& e);
    private:
        std::unique_ptr<Window> m_Window;
        ApplicationSpecification m_specification;

        bool m_isRunning = true;
        bool m_isMinimized = false;
    };

    // Implemented by CLIENT
    Application* CreateApplication(int argc, char** argv);
}