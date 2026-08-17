#include "input_converters.h"

int core::input::utils::ToGLFW(E_KEYS _key)
{
	switch (_key)
	{
		case E_KEYS::KEY_A:             return GLFW_KEY_A;
		case E_KEYS::KEY_B:             return GLFW_KEY_B;
		case E_KEYS::KEY_C:             return GLFW_KEY_C;
		case E_KEYS::KEY_D:             return GLFW_KEY_D;
		case E_KEYS::KEY_E:             return GLFW_KEY_E;
		case E_KEYS::KEY_F:             return GLFW_KEY_F;
		case E_KEYS::KEY_G:             return GLFW_KEY_G;
		case E_KEYS::KEY_H:             return GLFW_KEY_H;
		case E_KEYS::KEY_I:             return GLFW_KEY_I;
		case E_KEYS::KEY_J:             return GLFW_KEY_J;
		case E_KEYS::KEY_K:             return GLFW_KEY_K;
		case E_KEYS::KEY_L:             return GLFW_KEY_L;
		case E_KEYS::KEY_M:             return GLFW_KEY_M;
		case E_KEYS::KEY_N:             return GLFW_KEY_N;
		case E_KEYS::KEY_O:             return GLFW_KEY_O;
		case E_KEYS::KEY_P:             return GLFW_KEY_P;
		case E_KEYS::KEY_Q:             return GLFW_KEY_Q;
		case E_KEYS::KEY_R:             return GLFW_KEY_R;
		case E_KEYS::KEY_S:             return GLFW_KEY_S;
		case E_KEYS::KEY_T:             return GLFW_KEY_T;
		case E_KEYS::KEY_U:             return GLFW_KEY_U;
		case E_KEYS::KEY_V:             return GLFW_KEY_V;
		case E_KEYS::KEY_W:             return GLFW_KEY_W;
		case E_KEYS::KEY_X:             return GLFW_KEY_X;
		case E_KEYS::KEY_Y:             return GLFW_KEY_Y;
		case E_KEYS::KEY_Z:             return GLFW_KEY_Z;

		case E_KEYS::KEY_0:             return GLFW_KEY_0;
		case E_KEYS::KEY_1:             return GLFW_KEY_1;
		case E_KEYS::KEY_2:             return GLFW_KEY_2;
		case E_KEYS::KEY_3:             return GLFW_KEY_3;
		case E_KEYS::KEY_4:             return GLFW_KEY_4;
		case E_KEYS::KEY_5:             return GLFW_KEY_5;
		case E_KEYS::KEY_6:             return GLFW_KEY_6;
		case E_KEYS::KEY_7:             return GLFW_KEY_7;
		case E_KEYS::KEY_8:             return GLFW_KEY_8;
		case E_KEYS::KEY_9:             return GLFW_KEY_9;

		case E_KEYS::KEY_F1:            return GLFW_KEY_F1;
		case E_KEYS::KEY_F2:            return GLFW_KEY_F2;
		case E_KEYS::KEY_F3:            return GLFW_KEY_F3;
		case E_KEYS::KEY_F4:            return GLFW_KEY_F4;
		case E_KEYS::KEY_F5:            return GLFW_KEY_F5;
		case E_KEYS::KEY_F6:            return GLFW_KEY_F6;
		case E_KEYS::KEY_F7:            return GLFW_KEY_F7;
		case E_KEYS::KEY_F8:            return GLFW_KEY_F8;
		case E_KEYS::KEY_F9:            return GLFW_KEY_F9;
		case E_KEYS::KEY_F10:           return GLFW_KEY_F10;
		case E_KEYS::KEY_F11:           return GLFW_KEY_F11;
		case E_KEYS::KEY_F12:           return GLFW_KEY_F12;

		case E_KEYS::KEY_UP:            return GLFW_KEY_UP;
		case E_KEYS::KEY_DOWN:          return GLFW_KEY_DOWN;
		case E_KEYS::KEY_LEFT:          return GLFW_KEY_LEFT;
		case E_KEYS::KEY_RIGHT:         return GLFW_KEY_RIGHT;

		case E_KEYS::KEY_LEFT_SHIFT:    return GLFW_KEY_LEFT_SHIFT;
		case E_KEYS::KEY_RIGHT_SHIFT:   return GLFW_KEY_RIGHT_SHIFT;
		case E_KEYS::KEY_LEFT_CTRL:     return GLFW_KEY_LEFT_CONTROL;
		case E_KEYS::KEY_RIGHT_CTRL:    return GLFW_KEY_RIGHT_CONTROL;
		case E_KEYS::KEY_LEFT_ALT:      return GLFW_KEY_LEFT_ALT;
		case E_KEYS::KEY_RIGHT_ALT:     return GLFW_KEY_RIGHT_ALT;
		case E_KEYS::KEY_LEFT_SUPER:    return GLFW_KEY_LEFT_SUPER;
		case E_KEYS::KEY_RIGHT_SUPER:   return GLFW_KEY_RIGHT_SUPER;

		case E_KEYS::KEY_ESCAPE:        return GLFW_KEY_ESCAPE;
		case E_KEYS::KEY_ENTER:         return GLFW_KEY_ENTER;
		case E_KEYS::KEY_TAB:           return GLFW_KEY_TAB;
		case E_KEYS::KEY_BACKSPACE:     return GLFW_KEY_BACKSPACE;
		case E_KEYS::KEY_DELETE:        return GLFW_KEY_DELETE;
		case E_KEYS::KEY_INSERT:        return GLFW_KEY_INSERT;
		case E_KEYS::KEY_HOME:          return GLFW_KEY_HOME;
		case E_KEYS::KEY_END:           return GLFW_KEY_END;
		case E_KEYS::KEY_PAGE_UP:       return GLFW_KEY_PAGE_UP;
		case E_KEYS::KEY_PAGE_DOWN:     return GLFW_KEY_PAGE_DOWN;
		case E_KEYS::KEY_CAPS_LOCK:     return GLFW_KEY_CAPS_LOCK;
		case E_KEYS::KEY_SPACE:         return GLFW_KEY_SPACE;
		case E_KEYS::KEY_PRINT_SCREEN:  return GLFW_KEY_PRINT_SCREEN;
		case E_KEYS::KEY_SCROLL_LOCK:   return GLFW_KEY_SCROLL_LOCK;
		case E_KEYS::KEY_PAUSE:         return GLFW_KEY_PAUSE;
		case E_KEYS::KEY_MENU:          return GLFW_KEY_MENU;

		case E_KEYS::KEY_KP_0:          return GLFW_KEY_KP_0;
		case E_KEYS::KEY_KP_1:          return GLFW_KEY_KP_1;
		case E_KEYS::KEY_KP_2:          return GLFW_KEY_KP_2;
		case E_KEYS::KEY_KP_3:          return GLFW_KEY_KP_3;
		case E_KEYS::KEY_KP_4:          return GLFW_KEY_KP_4;
		case E_KEYS::KEY_KP_5:          return GLFW_KEY_KP_5;
		case E_KEYS::KEY_KP_6:          return GLFW_KEY_KP_6;
		case E_KEYS::KEY_KP_7:          return GLFW_KEY_KP_7;
		case E_KEYS::KEY_KP_8:          return GLFW_KEY_KP_8;
		case E_KEYS::KEY_KP_9:          return GLFW_KEY_KP_9;
		case E_KEYS::KEY_KP_DECIMAL:    return GLFW_KEY_KP_DECIMAL;
		case E_KEYS::KEY_KP_DIVIDE:     return GLFW_KEY_KP_DIVIDE;
		case E_KEYS::KEY_KP_MULTIPLY:   return GLFW_KEY_KP_MULTIPLY;
		case E_KEYS::KEY_KP_SUBTRACT:   return GLFW_KEY_KP_SUBTRACT;
		case E_KEYS::KEY_KP_ADD:        return GLFW_KEY_KP_ADD;
		case E_KEYS::KEY_KP_ENTER:      return GLFW_KEY_KP_ENTER;
		case E_KEYS::KEY_KP_EQUAL:      return GLFW_KEY_KP_EQUAL;
		case E_KEYS::KEY_NUM_LOCK:      return GLFW_KEY_NUM_LOCK;

		case E_KEYS::KEY_APOSTROPHE:    return GLFW_KEY_APOSTROPHE;
		case E_KEYS::KEY_COMMA:         return GLFW_KEY_COMMA;
		case E_KEYS::KEY_MINUS:         return GLFW_KEY_MINUS;
		case E_KEYS::KEY_PERIOD:        return GLFW_KEY_PERIOD;
		case E_KEYS::KEY_SLASH:         return GLFW_KEY_SLASH;
		case E_KEYS::KEY_SEMICOLON:     return GLFW_KEY_SEMICOLON;
		case E_KEYS::KEY_EQUAL:         return GLFW_KEY_EQUAL;
		case E_KEYS::KEY_LEFT_BRACKET:  return GLFW_KEY_LEFT_BRACKET;
		case E_KEYS::KEY_BACKSLASH:     return GLFW_KEY_BACKSLASH;
		case E_KEYS::KEY_RIGHT_BRACKET: return GLFW_KEY_RIGHT_BRACKET;
		case E_KEYS::KEY_GRAVE_ACCENT:  return GLFW_KEY_GRAVE_ACCENT;

		default:                        return GLFW_KEY_UNKNOWN;
	}
}

int core::input::utils::ToGLFW(E_MOUSE_BUTTON _mb)
{
	switch (_mb)
	{
		case E_MOUSE_BUTTON::LEFT:   return GLFW_MOUSE_BUTTON_1;
		case E_MOUSE_BUTTON::RIGHT:  return GLFW_MOUSE_BUTTON_2;
		case E_MOUSE_BUTTON::CENTER: return GLFW_MOUSE_BUTTON_3;
		default:                        return GLFW_KEY_UNKNOWN;
	}
}

WORD core::input::utils::ToXInput(E_CONTROLLER_KEYS _key)
{
	switch (_key)
	{
		case E_CONTROLLER_KEYS::KEY_A:              return XINPUT_GAMEPAD_A;
		case E_CONTROLLER_KEYS::KEY_B:              return XINPUT_GAMEPAD_B;
		case E_CONTROLLER_KEYS::KEY_X:              return XINPUT_GAMEPAD_X;
		case E_CONTROLLER_KEYS::KEY_Y:              return XINPUT_GAMEPAD_Y;

		case E_CONTROLLER_KEYS::KEY_LEFT_BUMPER:    return XINPUT_GAMEPAD_LEFT_SHOULDER;
		case E_CONTROLLER_KEYS::KEY_RIGHT_BUMPER:   return XINPUT_GAMEPAD_RIGHT_SHOULDER;

		case E_CONTROLLER_KEYS::KEY_START:          return XINPUT_GAMEPAD_START;
		case E_CONTROLLER_KEYS::KEY_BACK:           return XINPUT_GAMEPAD_BACK;

		case E_CONTROLLER_KEYS::KEY_LEFT_THUMB:     return XINPUT_GAMEPAD_LEFT_THUMB;
		case E_CONTROLLER_KEYS::KEY_RIGHT_THUMB:    return XINPUT_GAMEPAD_RIGHT_THUMB;

		case E_CONTROLLER_KEYS::KEY_DPAD_UP:        return XINPUT_GAMEPAD_DPAD_UP;
		case E_CONTROLLER_KEYS::KEY_DPAD_DOWN:      return XINPUT_GAMEPAD_DPAD_DOWN;
		case E_CONTROLLER_KEYS::KEY_DPAD_LEFT:      return XINPUT_GAMEPAD_DPAD_LEFT;
		case E_CONTROLLER_KEYS::KEY_DPAD_RIGHT:     return XINPUT_GAMEPAD_DPAD_RIGHT;

		default:                                    return 0;
	}
}