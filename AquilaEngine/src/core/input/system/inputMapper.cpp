#include <core/input/system/inputMapper.h>
#include <core/input/system/inputSystem.h>

#include <algorithm>

using namespace core::input::system;

InputMapper::InputMapper()
{
};

InputAction& InputMapper::Action(const std::string& _name)
{
    if (m_contextStack.empty())
        return m_contexts[m_currentContext][_name];

    return m_contexts[m_contextStack.back()][_name];
}

void InputMapper::SetContext(const std::string& _context)
{
    m_currentContext = _context;
}

void InputMapper::PushContext(const std::string& _context)
{
    m_contextStack.push_back(_context);
}

void InputMapper::PopContext()
{
    if (!m_contextStack.empty())
        m_contextStack.pop_back();
}

void InputMapper::Update(InputSystem& inputSystem)
{
    for (auto& contextName : m_contextStack)
    {
        auto it = m_contexts.find(contextName);
        if (it == m_contexts.end()) continue;

        for (auto& [name, action] : it->second)
        {
            if (action.bindings.empty())
            {
                continue;
            }

            action.m_triggered = false;
            action.m_value = 0.0f;

            if (action.axisCallBack || !action.bindings.empty())
            {
                bool hasAxis = std::any_of(action.bindings.begin(), action.bindings.end(),
                    [](const InputBinding& b) { return b.mode == E_INPUT_MODE::E_AXIS; });

                if (hasAxis)
                {
                    float axisValue = 0.0f;

                    for (auto& binding : action.bindings)
                    {
                        if (binding.mode != E_INPUT_MODE::E_AXIS) continue;

                        for (auto& [input, value] : binding.inputs)
                        {
                            bool held = std::visit([&](auto&& key) -> bool
                                {
                                    return inputSystem.IsKeyHeld(key);
                                }, input);

                            if (held) axisValue += value;
                        }
                    }

                    axisValue = std::clamp(axisValue, -1.0f, 1.0f);

                    action.m_value = axisValue;
                    action.m_triggered = axisValue != 0.0f;

                    if (action.axisCallBack)
                        action.axisCallBack(axisValue);

                    continue;
                }
            }

            bool triggered = false;

            for (auto& binding : action.bindings)
            {
                if (binding.inputs.empty()) continue;

                if (binding.mode == E_INPUT_MODE::E_COMBO)
                {
                    bool allValid = true;
                    for (auto& [input, value] : binding.inputs)
                    {
                        bool valid = std::visit([&](auto&& key) -> bool
                            {
                                if (binding.status == E_KEY_STATUS::E_PRESSED)  return inputSystem.IsKeyPressed(key);
                                if (binding.status == E_KEY_STATUS::E_HELD)     return inputSystem.IsKeyHeld(key);
                                if (binding.status == E_KEY_STATUS::E_RELEASED) return inputSystem.IsKeyReleased(key);
                                return false;
                            }, input);

                        if (!valid) { allValid = false; break; }
                    }
                    if (allValid) { triggered = true; break; }
                }
                else if (binding.mode == E_INPUT_MODE::E_SINGLE)
                {
                    bool valid = std::visit([&](auto&& key) -> bool
                        {
                            if (binding.status == E_KEY_STATUS::E_PRESSED)  return inputSystem.IsKeyPressed(key);
                            if (binding.status == E_KEY_STATUS::E_HELD)     return inputSystem.IsKeyHeld(key);
                            if (binding.status == E_KEY_STATUS::E_RELEASED) return inputSystem.IsKeyReleased(key);
                            return false;
                        }, binding.inputs[0].first);

                    if (valid) { triggered = true; break; }
                }
            }

            action.m_triggered = triggered;
            action.m_value = triggered ? 1.0f : 0.0f;

            if (triggered && action.callBack)
            {
                action.callBack();
            }
        }
    }
}