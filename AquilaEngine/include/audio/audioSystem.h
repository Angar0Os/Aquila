#ifndef AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_H
#define AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_H
#pragma once

#include <string>
#include <memory>

namespace audio
{
    struct MusicInfo
    {
        std::string path;
        float       baseVolume  = 1.0f;
        float       tempo       = 120.0f; 
    };

    class AudioSystem
    {
    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;

    public:
        AudioSystem();
        ~AudioSystem();

        AudioSystem(const AudioSystem&) = delete;
        AudioSystem& operator=(const AudioSystem&) = delete;

        void Play(const MusicInfo& info);
        void Pause();
        void Restart();

        void SeekRows(double deltaRows);
        bool IsPlaying() const;
        double GetCurrentRow() const;
        void SetVolume(float volume);
    };
}

#endif // AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_H