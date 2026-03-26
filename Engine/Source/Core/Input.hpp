#pragma once

#include <SDL3/SDL.h>

namespace Vanta {

    class Input
    {
    public:
        static bool IsKeyPressed(SDL_Scancode scancode);
        static bool IsMouseButtonPressed(uint8_t button);

        static float GetMouseX();
        static float GetMouseY();
        static std::pair<float, float> GetMousePosition();
    };

}