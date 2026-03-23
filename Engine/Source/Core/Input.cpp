#include "vapch.hpp"
#include "Input.hpp"

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
        float x, y;
        SDL_GetMouseState(&x, &y);
        return x;
    }

    float Input::GetMouseY()
    {
        float x, y;
        SDL_GetMouseState(&x, &y);
        return y;
    }

    void Input::GetMousePosition(float& x, float& y)
    {
        SDL_GetMouseState(&x, &y);
    }

}