#ifndef AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_SYSTEM_H
#define AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_SYSTEM_H
#pragma once

#include <memory>
#include <core/input/utils/keys.h>

#include <GLFW/glfw3.h>

namespace core::input::system
{
    /*! @brief Handles raw input polling for keyboard, mouse buttons, position, scroll and controller. */
    class InputSystem
    {
    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

    public:
        /*! @brief Constructs the input system and binds it to a GLFW window.
         *  @param[in] window The GLFW window to poll input from.
         */
        explicit InputSystem(GLFWwindow* window);

        ~InputSystem();

        /*! @brief Returns true if the key was just pressed this frame.
         *  @param[in] _key The key to query.
         */
        bool IsKeyPressed(core::input::utils::E_KEYS _key);

        /*! @brief Returns true if the key is being held down.
         *  @param[in] _key The key to query.
         */
        bool IsKeyHeld(core::input::utils::E_KEYS _key);

        /*! @brief Returns true if the key was just released this frame.
         *  @param[in] _key The key to query.
         */
        bool IsKeyReleased(core::input::utils::E_KEYS _key);

        /*! @brief Returns true if the mouse button was just pressed this frame.
         *  @param[in] _key The mouse button to query.
         */
        bool IsKeyPressed(core::input::utils::E_MOUSE_BUTTON _key);

        /*! @brief Returns true if the mouse button is being held down.
         *  @param[in] _key The mouse button to query.
         */
        bool IsKeyHeld(core::input::utils::E_MOUSE_BUTTON _key);

        /*! @brief Returns true if the mouse button was just released this frame.
         *  @param[in] _key The mouse button to query.
         */
        bool IsKeyReleased(core::input::utils::E_MOUSE_BUTTON _key);

        /*! @brief Returns true if the controller button was just pressed this frame.
         *  @param[in] _key The controller key to query.
         */

        bool IsKeyPressed(utils::E_CONTROLLER_KEYS _key);

        /*! @brief Returns true if the controller button is being held down.
         *  @param[in] _key The controller key to query.
         */
        bool IsKeyHeld(utils::E_CONTROLLER_KEYS _key);

        /*! @brief Returns true if the controller button was just released this frame.
         *  @param[in] _key The controller key to query.
         */
        bool IsKeyReleased(utils::E_CONTROLLER_KEYS _key);

        /*! @brief Returns the raw axis value [-1..1] for an analog controller input.
         *  @note For AXIS_LEFT/RIGHT_TRIGGER, range is [0..1].
         *  @param[in] _key The axis to query (AXIS_* or KEY_LEFT/RIGHT_TRIGGER).
         *  @return Normalized axis value.
         */
        float GetAxisValue(utils::E_CONTROLLER_KEYS _key);

        /*! @brief Returns the current mouse cursor position in screen coordinates.
         *  @return Pair of { x, y } position.
         */
       
        std::pair<double, double> GetMousePosition();

        /*! @brief Returns the mouse movement delta since the last frame.
         *  @return Pair of { deltaX, deltaY }.
         */
        std::pair<double, double> GetMouseDelta() const;

        /*! @brief Returns the scroll wheel delta since the last frame.
         *  @return Pair of { deltaX, deltaY }.
         */
        std::pair<double, double> GetScrollDelta() const;

        /*! @brief Polls all input states. Must be called once per frame. */
        void Update();

        /*! @brief Returns the platform-specific implementation.
         *  @return Reference to the internal Impl struct.
         */
        Impl& GetImpl() const;
    };
}

#endif //AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_SYSTEM_H