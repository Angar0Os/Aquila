#ifndef AQUILA_ENGINE_CORE_INPUT_UTILS_CONVERTERS_H
#define AQUILA_ENGINE_CORE_INPUT_UTILS_CONVERTERS_H
#pragma once

#include <GLFW/glfw3.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>   
#include <Xinput.h>

#include <core/input/utils/keys.h>

namespace core::input::utils
{
    int     ToGLFW(E_KEYS _key);
    int     ToGLFW(E_MOUSE_BUTTON _mb);
    WORD    ToXInput(E_CONTROLLER_KEYS _key);
}

#endif //AQUILA_ENGINE_CORE_INPUT_UTILS_CONVERTERS_H