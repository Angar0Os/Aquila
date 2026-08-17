#ifndef AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_ACTION_H
#define AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_ACTION_H
#pragma once

#include <core/input/utils/keys.h>

#include <vector>
#include <variant>
#include <functional>

using KEY_OR_MOUSE = std::variant<core::input::utils::E_KEYS, core::input::utils::E_MOUSE_BUTTON, core::input::utils::E_CONTROLLER_KEYS>;

namespace core::input::system
{
    /*! @brief Defines how multiple inputs are combined for an action. */
    enum class E_INPUT_MODE
    {
        E_SINGLE,    /*!< Each input triggers the action independently. */
        E_COMBO,     /*!< All inputs must be active simultaneously. */
        E_SEQUENCE,  /*!< Inputs must be pressed in order. */
        E_AXIS,      /*!< Inputs map to a [-1..1] float value. */

        Count
    };

    /*! @brief Defines the key state required to trigger an action. */
    enum class E_KEY_STATUS
    {
        E_HELD,      /*!< Key is being held down. */
        E_PRESSED,   /*!< Key was just pressed this frame. */
        E_RELEASED   /*!< Key was just released this frame. */
    };

    /*! @brief Represents a single binding � a set of inputs with a mode and a status. */
    struct InputBinding
    {
        std::vector<std::pair<KEY_OR_MOUSE, float>> inputs;  /*!< Inputs and their axis contribution. */
        E_INPUT_MODE                                mode;    /*!< How inputs are combined. */
        E_KEY_STATUS                                status;  /*!< Required key state to trigger. */
    };

    /*! @brief Represents a mappable action with one or more bindings and an optional callback. */
    struct InputAction
    {
        std::vector<InputBinding>   bindings;           /*!< List of bindings that can trigger this action. */
        std::function<void()>       callBack;           /*!< Callback for digital actions. */
        std::function<void(float)>  axisCallBack;       /*!< Callback for axis actions, receives [-1..1]. */

        bool  m_triggered = false;  /*!< Whether the action was triggered this frame. */
        float m_value = 0.0f;       /*!< Current value of the action, 1.0 or [-1..1] for axes. */

        /*! @brief Returns whether the action was triggered this frame.
         *  @return True if triggered, false otherwise.
         */
        bool IsTriggered() const { return m_triggered; }

        /*! @brief Returns the current value of the action.
         *  @return 1.0 for digital actions, [-1..1] for axis actions.
         */
        float GetValue() const { return m_value; }

        /*! @brief Binds a single input to this action with E_PRESSED as default status.
         *  @param[in] _input The key or mouse button to bind.
         *  @return Reference to self.
         */
        InputAction& operator=(KEY_OR_MOUSE _input);

        /*! @brief Sets a callback for digital actions (Single, Combo).
         *  @param[in] _callBack Function called when the action is triggered.
         *  @return Reference to self.
         */
        InputAction& operator=(std::function<void()> _callBack);

        /*! @brief Sets a callback for axis actions, receives the current axis value.
         *  @param[in] _callBack Function called each frame with the axis value [-1..1].
         *  @return Reference to self.
         */
        InputAction& operator=(std::function<void(float)> _callBack);

        /*! @brief Adds or updates a binding with explicit mode and status.
         *  @param[in] _binding Tuple of { inputs, mode, status }.
         *  @return Reference to self.
         */
        InputAction& operator+=(const std::tuple<std::vector<KEY_OR_MOUSE>, E_INPUT_MODE, E_KEY_STATUS>& _binding);

        /*! @brief Binds axis inputs with explicit values.
         *  @param[in] _axisInputs List of { input, axisValue } pairs, e.g. { {KEY_A, -1.0f}, {KEY_D, +1.0f} }.
         *  @return Reference to self.
         */
        InputAction& operator=(const std::vector<std::pair<KEY_OR_MOUSE, float>>& _axisInputs);
    };
}

#endif //AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_ACTION_H