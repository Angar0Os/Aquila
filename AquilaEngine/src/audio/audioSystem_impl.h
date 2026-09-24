#ifndef AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_IMPL_H
#define AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_IMPL_H
#pragma once

#include <audio/audioSystem.h>
#include <miniaudio/miniaudio.h>

namespace audio
{
    struct AudioSystem::Impl
    {
        ma_engine engine{};
        ma_sound  sound{};

        bool engineInitialized = false;
        bool soundLoaded = false;
        bool isPaused = false;

        MusicInfo currentInfo;
        ma_uint32 sampleRate = 0;

        ~Impl(); 
    };
}

#endif // AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_IMPL_H