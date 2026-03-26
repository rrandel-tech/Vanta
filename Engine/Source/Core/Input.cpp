#include "vapch.hpp"
#include "Input.hpp"

#include "Window.hpp"

namespace Vanta {

    bool Input::IsKeyPressed(SDL_Scancode scancode)
    {
        const bool* state = SDL_GetKeyboardState(nullptr);
        return state[scancode];
    }

    bool Input::IsMouseButtonPressed(uint8_t button)
    {
        Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);
        return (mouseState & SDL_BUTTON_MASK(button)) != 0;
    }

    float Input::GetMouseX()
    {
        auto [x, y] = GetMousePosition();
        return (float)x;
    }

    float Input::GetMouseY()
    {
        auto [x, y] = GetMousePosition();
        return (float)y;
    }

    std::pair<float, float> Input::GetMousePosition()
    {
        float x, y;
        SDL_GetMouseState(&x, &y);
        return { x, y };
    }

}