#ifndef AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_SYSTEM_WIN32_IMPL_H
#define AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_SYSTEM_WIN32_IMPL_H
#pragma once

#include <core/input/system/inputSystem.h>
#include <array>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>

namespace core::input::system
{
    struct InputSystem::Impl
    {
        explicit Impl(GLFWwindow* _window);

        std::array<bool, size_t(utils::E_KEYS::Count)> KeyPressed = {};
        std::array<bool, size_t(utils::E_KEYS::Count)> KeyHeld = {};
        std::array<bool, size_t(utils::E_KEYS::Count)> KeyReleased = {};

        std::array<bool, size_t(utils::E_MOUSE_BUTTON::Count)> MouseButtonPressed = {};
        std::array<bool, size_t(utils::E_MOUSE_BUTTON::Count)> MouseButtonHeld = {};
        std::array<bool, size_t(utils::E_MOUSE_BUTTON::Count)> MouseButtonReleased = {};

        std::array<bool, size_t(utils::E_CONTROLLER_KEYS::KEY_DPAD_RIGHT) + 1> ControllerPressed = {};
        std::array<bool, size_t(utils::E_CONTROLLER_KEYS::KEY_DPAD_RIGHT) + 1> ControllerHeld = {};
        std::array<bool, size_t(utils::E_CONTROLLER_KEYS::KEY_DPAD_RIGHT) + 1> ControllerReleased = {};

        XINPUT_STATE controllerState = {};
        bool controllerConnected = false;

        std::pair<double, double> mousePosition = { 0.0, 0.0 };
        std::pair<double, double> mouseDelta = { 0.0, 0.0 };

        std::pair<double, double> scrollDelta = { 0.0, 0.0 };

        GLFWwindow* window;

        static void ScrollCallback(GLFWwindow* _window, double _xOffset, double _yOffset);
    };
}

#endif //AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_SYSTEM_WIN32_IMPL_H