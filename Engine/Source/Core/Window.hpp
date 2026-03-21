#pragma once

#include "Events/Event.hpp"

#include <SDL3/SDL.h>

#include <functional>
#include <memory>

namespace Vanta {

    enum class WindowMode
    {
        Windowed = 0,
        BorderlessFullscreen,
        ExclusiveFullscreen
    };

    struct WindowSpecification
    {
        std::string Title = "Vanta";
        uint32_t Width = 1600, Height = 900;
        WindowMode Mode = WindowMode::Windowed;
        bool VSync = true;
    };

    class Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        Window(const WindowSpecification& specification);
        ~Window();

        void Init();
        void ProcessEvents();
        void SwapBuffers();

        uint32_t GetWidth() const { return m_data.width; }
        uint32_t GetHeight() const { return m_data.height; }

        virtual std::pair<uint32_t, uint32_t> GetSize() const { return { m_data.width, m_data.height }; }
        virtual std::pair<float, float> GetWindowPos() const;

        // Window attributes
        void SetEventCallback(const EventCallbackFn& callback) { m_data.eventCallback = callback; }

        void SetVSync(bool enabled);
        bool IsVSync() const;
        void SetResizable(bool resizable) const;

        void Maximize();
        void CenterWindow();

        const std::string& GetTitle() const { return m_data.title; }
        void SetTitle(const std::string& title);

        static std::unique_ptr<Window> Create(const WindowSpecification& specification = WindowSpecification());
    private:
        void PollEvents();
        void Shutdown();
    private:
        SDL_Window* m_Window = nullptr;
        SDL_Event m_Event {};

        WindowSpecification m_specification;
        struct WindowData
        {
            std::string title;
            uint32_t width, height;

            EventCallbackFn eventCallback;
        }; WindowData m_data;
    };

}