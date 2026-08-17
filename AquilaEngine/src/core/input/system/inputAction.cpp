#include <core/input/system/inputAction.h>

using namespace core::input::system;

InputAction& InputAction::operator=(KEY_OR_MOUSE _input)
{
    bindings.clear();
    bindings.push_back({ {{ _input, 0.0f }}, E_INPUT_MODE::E_SINGLE, E_KEY_STATUS::E_PRESSED });
    return *this;
}

InputAction& InputAction::operator=(std::function<void()> _callBack)
{
    callBack = _callBack;
    return *this;
}

InputAction& InputAction::operator=(std::function<void(float)> _callBack)
{
    axisCallBack = _callBack;
    return *this;
}

InputAction& InputAction::operator=(const std::vector<std::pair<KEY_OR_MOUSE, float>>& _axisInputs)
{
    bindings.clear();
    bindings.push_back({ _axisInputs, E_INPUT_MODE::E_AXIS, E_KEY_STATUS::E_HELD });
    return *this;
}

InputAction& InputAction::operator+=(const std::tuple<std::vector<KEY_OR_MOUSE>, E_INPUT_MODE, E_KEY_STATUS>& _binding)
{
    auto& [inputs, mode, status] = _binding;

    if (mode == E_INPUT_MODE::E_COMBO)
    {
        std::vector<std::pair<KEY_OR_MOUSE, float>> pairInputs;
        for (auto& input : inputs) pairInputs.push_back({ input, 0.0f });

        auto it = std::find_if(bindings.begin(), bindings.end(),
            [&](const InputBinding& b)
            {
                if (b.mode != E_INPUT_MODE::E_COMBO) return false;
                if (b.inputs.size() != pairInputs.size()) return false;
                for (int i = 0; i < b.inputs.size(); ++i)
                    if (b.inputs[i].first != pairInputs[i].first) return false;
                return true;
            });

        if (it != bindings.end())
        {
            it->status = status;
        }
        else
        {
            bindings.push_back({ pairInputs, mode, status });
        }
    }
    else
    {
        for (auto& input : inputs)
        {
            auto it = std::find_if(bindings.begin(), bindings.end(),
                [&](const InputBinding& b)
                {
                    return b.mode == E_INPUT_MODE::E_SINGLE && b.inputs[0].first == input;
                });

            if (it != bindings.end())
            {
                it->status = status;
            }
            else
            {
                bindings.push_back({ {{input, 0.0f}}, mode, status });
            }
        }
    }

    return *this;
}