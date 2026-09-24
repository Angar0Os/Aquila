#ifndef AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_MAPPER_H
#define AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_MAPPER_H
#pragma once

#include <string>
#include <unordered_map>

#include <core/input/system/inputAction.h>
#include <core/input/system/inputSystem.h>

namespace core::input::system
{
    /*! @brief Maps named actions to inputs, organized by contexts. */
    class InputMapper
    {
    public:
        explicit InputMapper();

        /*! @brief Sets the current context for action definition.
         *  @note Only affects Action() calls during setup, not runtime evaluation.
         *  @param[in] _context Name of the context to set as current.
         */
        void SetContext(const std::string& _context);

        /*! @brief Pushes a context onto the active stack.
         *  @note Only contexts in the stack are evaluated during Update().
         *  @param[in] _context Name of the context to activate.
         */
        void PushContext(const std::string& _context);

        /*! @brief Pops the top context from the active stack.
         *  @note Restores the previously active context.
         */
        void PopContext();

        /*! @brief Retrieves an action by name from the current context.
         *  @note During setup, uses m_currentContext. At runtime, uses the top of the stack.
         *  @param[in] _name Name of the action to retrieve.
         *  @return Reference to the action.
         */
        InputAction& Action(const std::string& _name);

        /*! @brief Evaluates all actions in the active contexts and triggers callbacks.
         *  @note Must be called once per frame after InputSystem::Update().
         *  @param[in] _inputSystem The input system to query key states from.
         */
        void Update(core::input::system::InputSystem& _inputSystem);

    private:
        std::string                                                                   m_currentContext = "Default"; /*!< Context used during action setup. */
        std::vector<std::string>                                                      m_contextStack;               /*!< Stack of active contexts evaluated in Update(). */
        std::unordered_map<std::string, std::unordered_map<std::string, InputAction>> m_contexts;                  /*!< All defined contexts and their actions. */
    };
}

#endif //AQUILA_ENGINE_CORE_INPUT_SYSTEM_INPUT_MAPPER_H