#include "audioSystem_impl.h"

#pragma comment(lib, "bass.lib")

using namespace audio;

AudioSystem::AudioSystem()
    : m_impl(new Impl)
{
    BASS_Init(
        -1,
        65000,
        BASS_DEVICE_STEREO,
        0,
        NULL
    );

    if (!BASS_Init)
    {
        MessageBox(
            0,
            (LPCWSTR)"BASS_Init could not be executed.",
            0,
            MB_ICONERROR
        );
    }

    if (HIWORD(BASS_GetVersion()) != BASSVERSION)
    {
        MessageBox(
            0,
            (LPCWSTR)
            "An incorrect version of BASS.DLL was loaded.",
            0,
            MB_ICONERROR
        );
    }
}

AudioSystem::~AudioSystem() = default;

void AudioSystem::Play(const MusicInfo& _info)
{
    m_impl->streamHandle =
        BASS_StreamCreateFile(
            false,
            _info.path.data(),
            0,
            0,
            0
        );

    m_impl->tempo = _info.tempo;
    m_impl->rowRate = (_info.tempo / 60.0f) / 8.0f;
    m_impl->volume = _info.baseVolume;

    BASS_ChannelPlay(m_impl->streamHandle, false);
    BASS_SetVolume(m_impl->volume);

    m_impl->isPlaying = true;
}

void AudioSystem::Pause()
{
    if (m_impl->isPlaying)
    {
        BASS_ChannelPause(m_impl->streamHandle);

        m_impl->isPlaying = false;
    }
    else
    {
        BASS_ChannelPlay(m_impl->streamHandle, false);

        m_impl->isPlaying = true;
    }
}

void AudioSystem::Restart()
{
    BASS_ChannelSetPosition(m_impl->streamHandle, 0, BASS_POS_BYTE);
    BASS_ChannelPlay(m_impl->streamHandle, false);

    m_impl->isPlaying = true;
}

void AudioSystem::SeekRows(double _rows)
{
    const double currentRow = GetCurrentRow();

    double targetRow = currentRow + _rows;

    if (targetRow < 0.0)
    {
        targetRow = 0.0;
    }

    const double time = targetRow / m_impl->rowRate;
    const QWORD position = BASS_ChannelSeconds2Bytes(m_impl->streamHandle, time);

    BASS_ChannelSetPosition(m_impl->streamHandle, position, BASS_POS_BYTE);
}

bool AudioSystem::IsPlaying()
{
    return m_impl->isPlaying;
}

double AudioSystem::GetCurrentRow()
{
    if (!m_impl->streamHandle)
    {
        return 0.0;
    }

    const QWORD position = BASS_ChannelGetPosition(m_impl->streamHandle, BASS_POS_BYTE);
    const double time = BASS_ChannelBytes2Seconds(m_impl->streamHandle, position);

    return time * m_impl->rowRate;
}