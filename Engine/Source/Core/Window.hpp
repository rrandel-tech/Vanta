#pragma once

#include "Events/Event.hpp"

#include <functional>
#include <memory>

namespace Vanta {

    struct WindowSpecification
    {
        std::string title = "Vanta";
        uint32_t width = 1600, height = 900;
    };

    class Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        Window(const WindowSpecification& specification);
        ~Window();

        void Init();

        uint32_t GetWidth() const { return m_data.width; }
        uint32_t GetHeight() const { return m_data.height; }

        // Window attributes
        void SetEventCallback(const EventCallbackFn& callback) { m_data.eventCallback = callback; }

        const std::string& GetTitle() const { return m_data.title; }
        void SetTitle(const std::string& title);

        static std::unique_ptr<Window> Create(const WindowSpecification& specification = WindowSpecification());
    private:
        void shutdown();
    private:
        WindowSpecification m_specification;
        struct WindowData
        {
            std::string title;
            uint32_t width, height;

            EventCallbackFn eventCallback;
        }; WindowData m_data;
    };

}