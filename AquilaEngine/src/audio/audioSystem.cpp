#include "audioSystem_impl.h"

#pragma comment(lib, "bass.lib")

using namespace audio;

AudioSystem::AudioSystem()
	: m_impl(new Impl)
{
	BASS_Init(-1, 65000, BASS_DEVICE_STEREO, 0, NULL);

	if (!BASS_Init)
	{
		MessageBox(0, (LPCWSTR)"BASS_Init could not be executed.", 0, MB_ICONERROR);
	}

	if (HIWORD(BASS_GetVersion()) != BASSVERSION)
	{
		MessageBox(0, (LPCWSTR)"An incorrect version of BASS.DLL was loaded.", 0, MB_ICONERROR);
	}
}

AudioSystem::~AudioSystem() = default;

void AudioSystem::Play(const MusicInfo& _info)
{
	m_impl->streamHandle = BASS_StreamCreateFile(false, _info.path.data(), 0, 0, 0);
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
		BASS_ChannelPlay(m_impl->streamHandle, false);
	}
	else
	{
		BASS_ChannelPause(m_impl->streamHandle);
	}
}

bool AudioSystem::IsPlaying()
{
	return m_impl->isPlaying;
}

double AudioSystem::GetCurrentRow()
{
	QWORD pos = BASS_ChannelGetPosition(m_impl->streamHandle, BASS_POS_BYTE);
	double time = BASS_ChannelBytes2Seconds(m_impl->streamHandle, pos);
	return time * m_impl->rowRate;
}



