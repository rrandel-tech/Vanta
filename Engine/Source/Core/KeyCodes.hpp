#pragma once
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_mouse.h>

namespace Vanta
{
    typedef enum class KeyCode : uint16_t
    {
        // Mapped to SDL_Scancode values
        Space        = SDL_SCANCODE_SPACE,
        Apostrophe   = SDL_SCANCODE_APOSTROPHE,
        Comma        = SDL_SCANCODE_COMMA,
        Minus        = SDL_SCANCODE_MINUS,
        Period       = SDL_SCANCODE_PERIOD,
        Slash        = SDL_SCANCODE_SLASH,

        D0 = SDL_SCANCODE_0,
        D1 = SDL_SCANCODE_1,
        D2 = SDL_SCANCODE_2,
        D3 = SDL_SCANCODE_3,
        D4 = SDL_SCANCODE_4,
        D5 = SDL_SCANCODE_5,
        D6 = SDL_SCANCODE_6,
        D7 = SDL_SCANCODE_7,
        D8 = SDL_SCANCODE_8,
        D9 = SDL_SCANCODE_9,

        Semicolon    = SDL_SCANCODE_SEMICOLON,
        Equal        = SDL_SCANCODE_EQUALS,

        A = SDL_SCANCODE_A,
        B = SDL_SCANCODE_B,
        C = SDL_SCANCODE_C,
        D = SDL_SCANCODE_D,
        E = SDL_SCANCODE_E,
        F = SDL_SCANCODE_F,
        G = SDL_SCANCODE_G,
        H = SDL_SCANCODE_H,
        I = SDL_SCANCODE_I,
        J = SDL_SCANCODE_J,
        K = SDL_SCANCODE_K,
        L = SDL_SCANCODE_L,
        M = SDL_SCANCODE_M,
        N = SDL_SCANCODE_N,
        O = SDL_SCANCODE_O,
        P = SDL_SCANCODE_P,
        Q = SDL_SCANCODE_Q,
        R = SDL_SCANCODE_R,
        S = SDL_SCANCODE_S,
        T = SDL_SCANCODE_T,
        U = SDL_SCANCODE_U,
        V = SDL_SCANCODE_V,
        W = SDL_SCANCODE_W,
        X = SDL_SCANCODE_X,
        Y = SDL_SCANCODE_Y,
        Z = SDL_SCANCODE_Z,

        LeftBracket  = SDL_SCANCODE_LEFTBRACKET,
        Backslash    = SDL_SCANCODE_BACKSLASH,
        RightBracket = SDL_SCANCODE_RIGHTBRACKET,
        GraveAccent  = SDL_SCANCODE_GRAVE,

        // Function keys
        Escape       = SDL_SCANCODE_ESCAPE,
        Enter        = SDL_SCANCODE_RETURN,
        Tab          = SDL_SCANCODE_TAB,
        Backspace    = SDL_SCANCODE_BACKSPACE,
        Insert       = SDL_SCANCODE_INSERT,
        Delete       = SDL_SCANCODE_DELETE,
        Right        = SDL_SCANCODE_RIGHT,
        Left         = SDL_SCANCODE_LEFT,
        Down         = SDL_SCANCODE_DOWN,
        Up           = SDL_SCANCODE_UP,
        PageUp       = SDL_SCANCODE_PAGEUP,
        PageDown     = SDL_SCANCODE_PAGEDOWN,
        Home         = SDL_SCANCODE_HOME,
        End          = SDL_SCANCODE_END,
        CapsLock     = SDL_SCANCODE_CAPSLOCK,
        ScrollLock   = SDL_SCANCODE_SCROLLLOCK,
        NumLock      = SDL_SCANCODE_NUMLOCKCLEAR,
        PrintScreen  = SDL_SCANCODE_PRINTSCREEN,
        Pause        = SDL_SCANCODE_PAUSE,

        F1  = SDL_SCANCODE_F1,
        F2  = SDL_SCANCODE_F2,
        F3  = SDL_SCANCODE_F3,
        F4  = SDL_SCANCODE_F4,
        F5  = SDL_SCANCODE_F5,
        F6  = SDL_SCANCODE_F6,
        F7  = SDL_SCANCODE_F7,
        F8  = SDL_SCANCODE_F8,
        F9  = SDL_SCANCODE_F9,
        F10 = SDL_SCANCODE_F10,
        F11 = SDL_SCANCODE_F11,
        F12 = SDL_SCANCODE_F12,
        F13 = SDL_SCANCODE_F13,
        F14 = SDL_SCANCODE_F14,
        F15 = SDL_SCANCODE_F15,
        F16 = SDL_SCANCODE_F16,
        F17 = SDL_SCANCODE_F17,
        F18 = SDL_SCANCODE_F18,
        F19 = SDL_SCANCODE_F19,
        F20 = SDL_SCANCODE_F20,
        F21 = SDL_SCANCODE_F21,
        F22 = SDL_SCANCODE_F22,
        F23 = SDL_SCANCODE_F23,
        F24 = SDL_SCANCODE_F24,

        // Keypad
        KP0        = SDL_SCANCODE_KP_0,
        KP1        = SDL_SCANCODE_KP_1,
        KP2        = SDL_SCANCODE_KP_2,
        KP3        = SDL_SCANCODE_KP_3,
        KP4        = SDL_SCANCODE_KP_4,
        KP5        = SDL_SCANCODE_KP_5,
        KP6        = SDL_SCANCODE_KP_6,
        KP7        = SDL_SCANCODE_KP_7,
        KP8        = SDL_SCANCODE_KP_8,
        KP9        = SDL_SCANCODE_KP_9,
        KPDecimal  = SDL_SCANCODE_KP_DECIMAL,
        KPDivide   = SDL_SCANCODE_KP_DIVIDE,
        KPMultiply = SDL_SCANCODE_KP_MULTIPLY,
        KPSubtract = SDL_SCANCODE_KP_MINUS,
        KPAdd      = SDL_SCANCODE_KP_PLUS,
        KPEnter    = SDL_SCANCODE_KP_ENTER,
        KPEqual    = SDL_SCANCODE_KP_EQUALS,

        LeftShift    = SDL_SCANCODE_LSHIFT,
        LeftControl  = SDL_SCANCODE_LCTRL,
        LeftAlt      = SDL_SCANCODE_LALT,
        LeftSuper    = SDL_SCANCODE_LGUI,
        RightShift   = SDL_SCANCODE_RSHIFT,
        RightControl = SDL_SCANCODE_RCTRL,
        RightAlt     = SDL_SCANCODE_RALT,
        RightSuper   = SDL_SCANCODE_RGUI,
        Menu         = SDL_SCANCODE_MENU,
    } Key;

    inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
    {
        os << static_cast<uint16_t>(keyCode);
        return os;
    }
}

// Convenience macros
#define VA_KEY_SPACE           ::Vanta::Key::Space
#define VA_KEY_APOSTROPHE      ::Vanta::Key::Apostrophe
#define VA_KEY_COMMA           ::Vanta::Key::Comma
#define VA_KEY_MINUS           ::Vanta::Key::Minus
#define VA_KEY_PERIOD          ::Vanta::Key::Period
#define VA_KEY_SLASH           ::Vanta::Key::Slash
#define VA_KEY_0               ::Vanta::Key::D0
#define VA_KEY_1               ::Vanta::Key::D1
#define VA_KEY_2               ::Vanta::Key::D2
#define VA_KEY_3               ::Vanta::Key::D3
#define VA_KEY_4               ::Vanta::Key::D4
#define VA_KEY_5               ::Vanta::Key::D5
#define VA_KEY_6               ::Vanta::Key::D6
#define VA_KEY_7               ::Vanta::Key::D7
#define VA_KEY_8               ::Vanta::Key::D8
#define VA_KEY_9               ::Vanta::Key::D9
#define VA_KEY_SEMICOLON       ::Vanta::Key::Semicolon
#define VA_KEY_EQUAL           ::Vanta::Key::Equal
#define VA_KEY_A               ::Vanta::Key::A
#define VA_KEY_B               ::Vanta::Key::B
#define VA_KEY_C               ::Vanta::Key::C
#define VA_KEY_D               ::Vanta::Key::D
#define VA_KEY_E               ::Vanta::Key::E
#define VA_KEY_F               ::Vanta::Key::F
#define VA_KEY_G               ::Vanta::Key::G
#define VA_KEY_H               ::Vanta::Key::H
#define VA_KEY_I               ::Vanta::Key::I
#define VA_KEY_J               ::Vanta::Key::J
#define VA_KEY_K               ::Vanta::Key::K
#define VA_KEY_L               ::Vanta::Key::L
#define VA_KEY_M               ::Vanta::Key::M
#define VA_KEY_N               ::Vanta::Key::N
#define VA_KEY_O               ::Vanta::Key::O
#define VA_KEY_P               ::Vanta::Key::P
#define VA_KEY_Q               ::Vanta::Key::Q
#define VA_KEY_R               ::Vanta::Key::R
#define VA_KEY_S               ::Vanta::Key::S
#define VA_KEY_T               ::Vanta::Key::T
#define VA_KEY_U               ::Vanta::Key::U
#define VA_KEY_V               ::Vanta::Key::V
#define VA_KEY_W               ::Vanta::Key::W
#define VA_KEY_X               ::Vanta::Key::X
#define VA_KEY_Y               ::Vanta::Key::Y
#define VA_KEY_Z               ::Vanta::Key::Z
#define VA_KEY_LEFT_BRACKET    ::Vanta::Key::LeftBracket
#define VA_KEY_BACKSLASH       ::Vanta::Key::Backslash
#define VA_KEY_RIGHT_BRACKET   ::Vanta::Key::RightBracket
#define VA_KEY_GRAVE_ACCENT    ::Vanta::Key::GraveAccent
#define VA_KEY_ESCAPE          ::Vanta::Key::Escape
#define VA_KEY_ENTER           ::Vanta::Key::Enter
#define VA_KEY_TAB             ::Vanta::Key::Tab
#define VA_KEY_BACKSPACE       ::Vanta::Key::Backspace
#define VA_KEY_INSERT          ::Vanta::Key::Insert
#define VA_KEY_DELETE          ::Vanta::Key::Delete
#define VA_KEY_RIGHT           ::Vanta::Key::Right
#define VA_KEY_LEFT            ::Vanta::Key::Left
#define VA_KEY_DOWN            ::Vanta::Key::Down
#define VA_KEY_UP              ::Vanta::Key::Up
#define VA_KEY_PAGE_UP         ::Vanta::Key::PageUp
#define VA_KEY_PAGE_DOWN       ::Vanta::Key::PageDown
#define VA_KEY_HOME            ::Vanta::Key::Home
#define VA_KEY_END             ::Vanta::Key::End
#define VA_KEY_CAPS_LOCK       ::Vanta::Key::CapsLock
#define VA_KEY_SCROLL_LOCK     ::Vanta::Key::ScrollLock
#define VA_KEY_NUM_LOCK        ::Vanta::Key::NumLock
#define VA_KEY_PRINT_SCREEN    ::Vanta::Key::PrintScreen
#define VA_KEY_PAUSE           ::Vanta::Key::Pause
#define VA_KEY_F1              ::Vanta::Key::F1
#define VA_KEY_F2              ::Vanta::Key::F2
#define VA_KEY_F3              ::Vanta::Key::F3
#define VA_KEY_F4              ::Vanta::Key::F4
#define VA_KEY_F5              ::Vanta::Key::F5
#define VA_KEY_F6              ::Vanta::Key::F6
#define VA_KEY_F7              ::Vanta::Key::F7
#define VA_KEY_F8              ::Vanta::Key::F8
#define VA_KEY_F9              ::Vanta::Key::F9
#define VA_KEY_F10             ::Vanta::Key::F10
#define VA_KEY_F11             ::Vanta::Key::F11
#define VA_KEY_F12             ::Vanta::Key::F12
#define VA_KEY_F13             ::Vanta::Key::F13
#define VA_KEY_F14             ::Vanta::Key::F14
#define VA_KEY_F15             ::Vanta::Key::F15
#define VA_KEY_F16             ::Vanta::Key::F16
#define VA_KEY_F17             ::Vanta::Key::F17
#define VA_KEY_F18             ::Vanta::Key::F18
#define VA_KEY_F19             ::Vanta::Key::F19
#define VA_KEY_F20             ::Vanta::Key::F20
#define VA_KEY_F21             ::Vanta::Key::F21
#define VA_KEY_F22             ::Vanta::Key::F22
#define VA_KEY_F23             ::Vanta::Key::F23
#define VA_KEY_F24             ::Vanta::Key::F24
#define VA_KEY_KP_0            ::Vanta::Key::KP0
#define VA_KEY_KP_1            ::Vanta::Key::KP1
#define VA_KEY_KP_2            ::Vanta::Key::KP2
#define VA_KEY_KP_3            ::Vanta::Key::KP3
#define VA_KEY_KP_4            ::Vanta::Key::KP4
#define VA_KEY_KP_5            ::Vanta::Key::KP5
#define VA_KEY_KP_6            ::Vanta::Key::KP6
#define VA_KEY_KP_7            ::Vanta::Key::KP7
#define VA_KEY_KP_8            ::Vanta::Key::KP8
#define VA_KEY_KP_9            ::Vanta::Key::KP9
#define VA_KEY_KP_DECIMAL      ::Vanta::Key::KPDecimal
#define VA_KEY_KP_DIVIDE       ::Vanta::Key::KPDivide
#define VA_KEY_KP_MULTIPLY     ::Vanta::Key::KPMultiply
#define VA_KEY_KP_SUBTRACT     ::Vanta::Key::KPSubtract
#define VA_KEY_KP_ADD          ::Vanta::Key::KPAdd
#define VA_KEY_KP_ENTER        ::Vanta::Key::KPEnter
#define VA_KEY_KP_EQUAL        ::Vanta::Key::KPEqual
#define VA_KEY_LEFT_SHIFT      ::Vanta::Key::LeftShift
#define VA_KEY_LEFT_CONTROL    ::Vanta::Key::LeftControl
#define VA_KEY_LEFT_ALT        ::Vanta::Key::LeftAlt
#define VA_KEY_LEFT_SUPER      ::Vanta::Key::LeftSuper
#define VA_KEY_RIGHT_SHIFT     ::Vanta::Key::RightShift
#define VA_KEY_RIGHT_CONTROL   ::Vanta::Key::RightControl
#define VA_KEY_RIGHT_ALT       ::Vanta::Key::RightAlt
#define VA_KEY_RIGHT_SUPER     ::Vanta::Key::RightSuper
#define VA_KEY_MENU            ::Vanta::Key::Menu

// Mouse buttons
#define VA_MOUSE_BUTTON_LEFT   SDL_BUTTON_LEFT
#define VA_MOUSE_BUTTON_RIGHT  SDL_BUTTON_RIGHT
#define VA_MOUSE_BUTTON_MIDDLE SDL_BUTTON_MIDDLE