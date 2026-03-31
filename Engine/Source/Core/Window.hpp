#pragma once

#include "Events/Event.hpp"

#include "Renderer/RendererContext.hpp"

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

    class VulkanSwapChain;

    class Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        Window(const WindowSpecification& specification);
        ~Window();

        void Init();
        void ProcessEvents();
        void SwapBuffers();

        uint32_t GetWidth() const { return m_Data.Width; }
        uint32_t GetHeight() const { return m_Data.Height; }

        virtual std::pair<uint32_t, uint32_t> GetSize() const { return { m_Data.Width, m_Data.Height }; }
        virtual std::pair<float, float> GetWindowPos() const;

        // Window attributes
        void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }

        void SetVSync(bool enabled);
        bool IsVSync() const;
        void SetResizable(bool resizable) const;

        void Maximize();
        void CenterWindow();

        const std::string& GetTitle() const { return m_Data.Title; }
        void SetTitle(const std::string& title);

        SDL_Window* GetNativeWindow() const { return m_Window; }

        Ref<RendererContext> GetRenderContext() { return m_RendererContext; }
        VulkanSwapChain& GetSwapChain();

        static Window* Create(const WindowSpecification& specification = WindowSpecification());
    private:
        void PollEvents();
        void Shutdown();
    private:
        SDL_Window* m_Window = nullptr;
        SDL_Event m_Event {};

        WindowSpecification m_Specification;
        struct WindowData
        {
            std::string Title;
            uint32_t Width, Height;

            EventCallbackFn EventCallback;
        }; WindowData m_Data;

        Ref<RendererContext> m_RendererContext;
        VulkanSwapChain* m_SwapChain;
    };

}