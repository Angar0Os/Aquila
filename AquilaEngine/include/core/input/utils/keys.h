#ifndef AQUILA_ENGINE_CORE_INPUT_UTILS_H
#define AQUILA_ENGINE_CORE_INPUT_UTILS_H
#pragma once

namespace core::input::utils
{
    enum class E_KEYS
    {
        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_M,
        KEY_N,
        KEY_O,
        KEY_P,
        KEY_Q,
        KEY_R,
        KEY_S,
        KEY_T,
        KEY_U,
        KEY_V,
        KEY_W,
        KEY_X,
        KEY_Y,
        KEY_Z,

        KEY_0,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,

        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,

        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,

        KEY_LEFT_SHIFT,
        KEY_RIGHT_SHIFT,
        KEY_LEFT_CTRL,
        KEY_RIGHT_CTRL,
        KEY_LEFT_ALT,
        KEY_RIGHT_ALT,
        KEY_LEFT_SUPER,
        KEY_RIGHT_SUPER,

        KEY_ESCAPE,
        KEY_ENTER,
        KEY_TAB,
        KEY_BACKSPACE,
        KEY_DELETE,
        KEY_INSERT,
        KEY_HOME,
        KEY_END,
        KEY_PAGE_UP,
        KEY_PAGE_DOWN,
        KEY_CAPS_LOCK,
        KEY_SPACE,
        KEY_PRINT_SCREEN,
        KEY_SCROLL_LOCK,
        KEY_PAUSE,
        KEY_MENU,

        KEY_KP_0,
        KEY_KP_1,
        KEY_KP_2,
        KEY_KP_3,
        KEY_KP_4,
        KEY_KP_5,
        KEY_KP_6,
        KEY_KP_7,
        KEY_KP_8,
        KEY_KP_9,
        KEY_KP_DECIMAL,
        KEY_KP_DIVIDE,
        KEY_KP_MULTIPLY,
        KEY_KP_SUBTRACT,
        KEY_KP_ADD,
        KEY_KP_ENTER,
        KEY_KP_EQUAL,
        KEY_NUM_LOCK,

        KEY_APOSTROPHE,
        KEY_COMMA,
        KEY_MINUS,
        KEY_PERIOD,
        KEY_SLASH,
        KEY_SEMICOLON,
        KEY_EQUAL,
        KEY_LEFT_BRACKET,
        KEY_BACKSLASH,
        KEY_RIGHT_BRACKET,
        KEY_GRAVE_ACCENT,

        Count
    };

    enum class E_MOUSE_BUTTON
    {
        LEFT,
        RIGHT,
        CENTER,

        Count
    };

    enum class E_CONTROLLER_KEYS
    {
        KEY_A,
        KEY_B,
        KEY_X,
        KEY_Y,

        KEY_LEFT_BUMPER,
        KEY_RIGHT_BUMPER,

        KEY_LEFT_TRIGGER,
        KEY_RIGHT_TRIGGER,

        KEY_START,
        KEY_BACK,
        KEY_GUIDE,

        KEY_LEFT_THUMB,
        KEY_RIGHT_THUMB,

        KEY_DPAD_UP,
        KEY_DPAD_DOWN,
        KEY_DPAD_LEFT,
        KEY_DPAD_RIGHT,

        AXIS_LEFT_X,
        AXIS_LEFT_Y,
        AXIS_RIGHT_X,
        AXIS_RIGHT_Y,

        AXIS_LEFT_TRIGGER,
        AXIS_RIGHT_TRIGGER,
    };
}

#endif //AQUILA_ENGINE_CORE_INPUT_UTILS_H