#define MINIAUDIO_IMPLEMENTATION   
#include "audioSystem_impl.h"

#include <algorithm>
#include <stdexcept>
#include <string>

using namespace audio;

constexpr double kRowsPerBeat = 4.0;

void ThrowIfFailed(ma_result result, const char* what)
{
    if (result != MA_SUCCESS)
    {
        throw std::runtime_error(
            std::string("AudioSystem: ") + what + " a echoue (" +
            ma_result_description(result) + ")"
        );
    }
}

double SecondsPerRow(float tempo)
{
    return 60.0 / ((double)tempo * kRowsPerBeat);
}
    
AudioSystem::Impl::~Impl()
{
    if (soundLoaded)
    {
        ma_sound_uninit(&sound);
    }
    if (engineInitialized)
    {
        ma_engine_uninit(&engine);
    }
}

AudioSystem::AudioSystem()
    : m_impl(std::make_unique<Impl>())
{
    ThrowIfFailed(ma_engine_init(nullptr, &m_impl->engine), "ma_engine_init");
    m_impl->engineInitialized = true;
}

AudioSystem::~AudioSystem() = default;

void AudioSystem::Play(const MusicInfo& info)
{
    if (m_impl->soundLoaded)
    {
        ma_sound_uninit(&m_impl->sound);
        m_impl->soundLoaded = false;
    }

    ThrowIfFailed(
        ma_sound_init_from_file(
            &m_impl->engine,
            info.path.c_str(),
            MA_SOUND_FLAG_DECODE,
            nullptr,
            nullptr,
            &m_impl->sound
        ),
        "ma_sound_init_from_file (fichier introuvable ou format non supporte)"
    );
    m_impl->soundLoaded = true;

    ThrowIfFailed(
        ma_sound_get_data_format(&m_impl->sound, nullptr, nullptr, &m_impl->sampleRate, nullptr, 0),
        "ma_sound_get_data_format"
    );

    m_impl->currentInfo = info;
    ma_sound_set_volume(&m_impl->sound, info.baseVolume);

    ThrowIfFailed(ma_sound_start(&m_impl->sound), "ma_sound_start");
    m_impl->isPaused = false;
}

void AudioSystem::Pause()
{
    if (!m_impl->soundLoaded) return;

    if (m_impl->isPaused)
    {
        ma_sound_start(&m_impl->sound);
    }
    else
    {
        ma_sound_stop(&m_impl->sound); 
    }
    m_impl->isPaused = !m_impl->isPaused;
}

void AudioSystem::Restart()
{
    if (!m_impl->soundLoaded) return;

    ma_sound_seek_to_pcm_frame(&m_impl->sound, 0);
    ma_sound_start(&m_impl->sound);
    m_impl->isPaused = false;
}

void AudioSystem::SeekRows(double deltaRows)
{
    if (!m_impl->soundLoaded) return;

    const double currentSeconds = GetCurrentRow() * SecondsPerRow(m_impl->currentInfo.tempo);
    const double targetSeconds = std::max(0.0, currentSeconds + deltaRows * SecondsPerRow(m_impl->currentInfo.tempo));

    ma_uint64 length = 0;
    ma_sound_get_length_in_pcm_frames(&m_impl->sound, &length);

    const ma_uint64 targetFrame = std::min<ma_uint64>(
        (ma_uint64)(targetSeconds * (double)m_impl->sampleRate),
        length
    );

    ma_sound_seek_to_pcm_frame(&m_impl->sound, targetFrame);

    if (!m_impl->isPaused)
    {
        ma_sound_start(&m_impl->sound);
    }
}

bool AudioSystem::IsPlaying() const
{
    return m_impl->soundLoaded && !m_impl->isPaused && !ma_sound_at_end(&m_impl->sound);
}

double AudioSystem::GetCurrentRow() const
{
    if (!m_impl->soundLoaded || m_impl->sampleRate == 0) return 0.0;

    ma_uint64 frame = 0;
    ma_sound_get_cursor_in_pcm_frames(&m_impl->sound, &frame);

    const double seconds = (double)frame / (double)m_impl->sampleRate;
    return seconds / SecondsPerRow(m_impl->currentInfo.tempo);
}

void AudioSystem::SetVolume(float volume)
{
    if (!m_impl->soundLoaded) return;
    ma_sound_set_volume(&m_impl->sound, volume);
}
