#pragma once

#include "KeyCodes.hpp"

#include <SDL3/SDL.h>

namespace Vanta {

    enum class CursorMode
    {
        Normal = 0,
        Hidden = 1,
        Locked = 2
    };

    typedef enum class MouseButton : uint8_t
    {
        Left   = SDL_BUTTON_LEFT,    // 1
        Middle = SDL_BUTTON_MIDDLE,  // 2
        Right  = SDL_BUTTON_RIGHT,   // 3
        X1     = SDL_BUTTON_X1,      // 4
        X2     = SDL_BUTTON_X2,      // 5
    } Button;

    inline std::ostream& operator<<(std::ostream& os, MouseButton button)
    {
        os << static_cast<uint8_t>(button);
        return os;
    }

    class Input
    {
    public:
        static bool IsKeyPressed(KeyCode key);
        static bool IsMouseButtonPressed(MouseButton button);

        static float GetMouseX();
        static float GetMouseY();
        static std::pair<float, float> GetMousePosition();

        static void SetCursorMode(CursorMode mode);
        static CursorMode GetCursorMode();
    };

}