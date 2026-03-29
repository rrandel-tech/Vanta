#include "vapch.hpp"
#include "Input.hpp"

#include "Window.hpp"
#include "Application.hpp"

namespace Vanta {

    bool Input::IsKeyPressed(KeyCode keycode)
    {
        const bool* state = SDL_GetKeyboardState(nullptr);
        return state[static_cast<uint16_t>(keycode)];
    }

    bool Input::IsMouseButtonPressed(MouseButton button)
    {
        SDL_MouseButtonFlags mouseState = SDL_GetMouseState(nullptr, nullptr);
        return (mouseState & SDL_BUTTON_MASK(static_cast<uint8_t>(button))) != 0;
    }

    float Input::GetMouseX()
    {
        auto [x, y] = GetMousePosition();
        return x;
    }

    float Input::GetMouseY()
    {
        auto [x, y] = GetMousePosition();
        return y;
    }

    std::pair<float, float> Input::GetMousePosition()
    {
        float x, y;
        SDL_GetMouseState(&x, &y);
        return { x, y };
    }

    void Input::SetCursorMode(CursorMode mode)
    {
        SDL_Window* nativeWindow = static_cast<SDL_Window*>(
            Application::Get().GetWindow().GetNativeWindow()
        );

        switch (mode)
        {
            case CursorMode::Normal:
                SDL_SetWindowRelativeMouseMode(nativeWindow, false);
                SDL_ShowCursor();
                break;
            case CursorMode::Hidden:
                SDL_SetWindowRelativeMouseMode(nativeWindow, false);
                SDL_HideCursor();
                break;
            case CursorMode::Locked:
                SDL_SetWindowRelativeMouseMode(nativeWindow, true);
                SDL_HideCursor();
                break;
        }
    }

    CursorMode Input::GetCursorMode()
    {
        SDL_Window* nativeWindow = static_cast<SDL_Window*>(
            Application::Get().GetWindow().GetNativeWindow()
        );

        if (SDL_GetWindowRelativeMouseMode(nativeWindow))
            return CursorMode::Locked;

        if (SDL_CursorVisible())
            return CursorMode::Normal;

        return CursorMode::Hidden;
    }

}