#include "inputSystem_Win32_impl.h"
#include "../utils/input_converters.h"

#pragma comment(lib, "Xinput9_1_0.lib")

#include <memory>

#include <GLFW/glfw3.h>

using namespace core::input::system;

void InputSystem::Impl::ScrollCallback(GLFWwindow* _window, double _xOffset, double _yOffset)
{
    auto* impl = static_cast<InputSystem::Impl*>(glfwGetWindowUserPointer(_window));
    if (impl)
    {
        impl->scrollDelta.first += _xOffset;
        impl->scrollDelta.second += _yOffset;
    }
}

InputSystem::Impl::Impl(GLFWwindow* _window)
    : window(_window)
{
    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, &InputSystem::Impl::ScrollCallback);
}

InputSystem::InputSystem(GLFWwindow* _window)
{
    m_impl = std::make_unique<Impl>(_window);
}

bool InputSystem::IsKeyPressed(core::input::utils::E_KEYS _key)
{
    return m_impl->KeyPressed[size_t(_key)];
}

bool InputSystem::IsKeyHeld(core::input::utils::E_KEYS _key)
{
    return m_impl->KeyHeld[size_t(_key)];
}

bool InputSystem::IsKeyReleased(core::input::utils::E_KEYS _key)
{
    return m_impl->KeyReleased[size_t(_key)];
}

bool InputSystem::IsKeyPressed(core::input::utils::E_MOUSE_BUTTON _key)
{
    return m_impl->MouseButtonPressed[size_t(_key)];
}

bool InputSystem::IsKeyHeld(core::input::utils::E_MOUSE_BUTTON _key)
{
    return m_impl->MouseButtonHeld[size_t(_key)];
}

bool InputSystem::IsKeyReleased(core::input::utils::E_MOUSE_BUTTON _key)
{
    return m_impl->MouseButtonReleased[size_t(_key)];
}

std::pair<double, double> InputSystem::GetMousePosition()
{
    return m_impl->mousePosition;
}

std::pair<double, double> InputSystem::GetMouseDelta() const
{
    return m_impl->mouseDelta;
}

std::pair<double, double> InputSystem::GetScrollDelta() const
{
    return m_impl->scrollDelta;
}

bool InputSystem::IsKeyPressed(utils::E_CONTROLLER_KEYS _key)
{
    return m_impl->ControllerPressed[size_t(_key)];
}

bool InputSystem::IsKeyHeld(utils::E_CONTROLLER_KEYS _key)
{
    return m_impl->ControllerHeld[size_t(_key)];
}

bool InputSystem::IsKeyReleased(utils::E_CONTROLLER_KEYS _key)
{
    return m_impl->ControllerReleased[size_t(_key)];
}

bool IsControllerButtonDown(const XINPUT_STATE& _state, core::input::utils::E_CONTROLLER_KEYS _key)
{
    using namespace core::input::utils;

    switch (_key)
    {
        case E_CONTROLLER_KEYS::KEY_A:            return _state.Gamepad.wButtons & XINPUT_GAMEPAD_A;
        case E_CONTROLLER_KEYS::KEY_B:            return _state.Gamepad.wButtons & XINPUT_GAMEPAD_B;
        case E_CONTROLLER_KEYS::KEY_X:            return _state.Gamepad.wButtons & XINPUT_GAMEPAD_X;
        case E_CONTROLLER_KEYS::KEY_Y:            return _state.Gamepad.wButtons & XINPUT_GAMEPAD_Y;
        case E_CONTROLLER_KEYS::KEY_LEFT_BUMPER:  return _state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
        case E_CONTROLLER_KEYS::KEY_RIGHT_BUMPER: return _state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
        case E_CONTROLLER_KEYS::KEY_START:        return _state.Gamepad.wButtons & XINPUT_GAMEPAD_START;
        case E_CONTROLLER_KEYS::KEY_BACK:         return _state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;
        case E_CONTROLLER_KEYS::KEY_LEFT_THUMB:   return _state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
        case E_CONTROLLER_KEYS::KEY_RIGHT_THUMB:  return _state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
        case E_CONTROLLER_KEYS::KEY_DPAD_UP:      return _state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
        case E_CONTROLLER_KEYS::KEY_DPAD_DOWN:    return _state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
        case E_CONTROLLER_KEYS::KEY_DPAD_LEFT:    return _state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
        case E_CONTROLLER_KEYS::KEY_DPAD_RIGHT:   return _state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
        case E_CONTROLLER_KEYS::KEY_LEFT_TRIGGER:  return _state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        case E_CONTROLLER_KEYS::KEY_RIGHT_TRIGGER: return _state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        default: return false;
    }
}

float Normalize(SHORT _raw)
{
    constexpr float deadzone = 0.15f;
    float value = std::max(-1.0f, static_cast<float>(_raw) / 32767.0f);
    if (std::abs(value) < deadzone) return 0.0f;
    return value;
}

float InputSystem::GetAxisValue(utils::E_CONTROLLER_KEYS _key)
{
    using namespace core::input::utils;
    const auto& pad = m_impl->controllerState.Gamepad;

    switch (_key)
    {
        case E_CONTROLLER_KEYS::AXIS_LEFT_X:  return Normalize(pad.sThumbLX);
        case E_CONTROLLER_KEYS::AXIS_LEFT_Y:  return Normalize(pad.sThumbLY);
        case E_CONTROLLER_KEYS::AXIS_RIGHT_X: return Normalize(pad.sThumbRX);
        case E_CONTROLLER_KEYS::AXIS_RIGHT_Y: return Normalize(pad.sThumbRY);
        case E_CONTROLLER_KEYS::AXIS_LEFT_TRIGGER:  return pad.bLeftTrigger / 255.0f;
        case E_CONTROLLER_KEYS::AXIS_RIGHT_TRIGGER: return pad.bRightTrigger / 255.0f;
        default: return 0.0f;
    }
}

void InputSystem::Update()
{
    m_impl->scrollDelta = { 0.0, 0.0 };

    glfwPollEvents();

    double newX, newY;
    glfwGetCursorPos(m_impl->window, &newX, &newY);
    m_impl->mouseDelta.first = newX - m_impl->mousePosition.first;
    m_impl->mouseDelta.second = newY - m_impl->mousePosition.second;
    m_impl->mousePosition = { newX, newY };

    for (int i = 0; i < size_t(core::input::utils::E_KEYS::Count); ++i)
    {
        m_impl->KeyPressed[i] = false;
        m_impl->KeyReleased[i] = false;

        auto glfwKey = core::input::utils::ToGLFW(static_cast<core::input::utils::E_KEYS>(i));

        if (glfwGetKey(m_impl->window, glfwKey) == GLFW_PRESS)
        {
            m_impl->KeyPressed[i] = !m_impl->KeyHeld[i];
            m_impl->KeyHeld[i] = true;
        }
        else if (m_impl->KeyHeld[i])
        {
            m_impl->KeyReleased[i] = true;
            m_impl->KeyHeld[i] = false;
        }
    }

    for (int i = 0; i < size_t(core::input::utils::E_MOUSE_BUTTON::Count); ++i)
    {
        m_impl->MouseButtonPressed[i] = false;
        m_impl->MouseButtonReleased[i] = false;

        auto glfwBtn = core::input::utils::ToGLFW(static_cast<core::input::utils::E_MOUSE_BUTTON>(i));

        if (glfwGetMouseButton(m_impl->window, glfwBtn) == GLFW_PRESS)
        {
            m_impl->MouseButtonPressed[i] = !m_impl->MouseButtonHeld[i];
            m_impl->MouseButtonHeld[i] = true;
        }
        else if (m_impl->MouseButtonHeld[i])
        {
            m_impl->MouseButtonReleased[i] = true;
            m_impl->MouseButtonHeld[i] = false;
        }
    }

    ZeroMemory(&m_impl->controllerState, sizeof(XINPUT_STATE));
    m_impl->controllerConnected = (XInputGetState(0, &m_impl->controllerState) == ERROR_SUCCESS);

    for (int i = 0; i <= size_t(core::input::utils::E_CONTROLLER_KEYS::KEY_DPAD_RIGHT); ++i)
    {
        m_impl->ControllerPressed[i] = false;
        m_impl->ControllerReleased[i] = false;

        bool isDown = m_impl->controllerConnected && IsControllerButtonDown(m_impl->controllerState, static_cast<core::input::utils::E_CONTROLLER_KEYS>(i));

        if (isDown)
        {
            m_impl->ControllerPressed[i] = !m_impl->ControllerHeld[i];
            m_impl->ControllerHeld[i] = true;
        }
        else if (m_impl->ControllerHeld[i])
        {
            m_impl->ControllerReleased[i] = true;
            m_impl->ControllerHeld[i] = false;
        }
    }
}

InputSystem::Impl& InputSystem::GetImpl() const
{
    return *m_impl;
}

InputSystem::~InputSystem() = default;