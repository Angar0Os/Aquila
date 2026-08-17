#ifndef AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_H
#define AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_H
#pragma once

// TODO : I think we will change this from bass.h to miniaudio or another F2U lib.

#include <string>
#include <memory>


namespace audio
{
	struct MusicInfo
	{
		std::string path;
		float baseVolume;
		float tempo;
	};

	class AudioSystem
	{
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	public:
		explicit AudioSystem();
		~AudioSystem() noexcept;

		void Play(const MusicInfo& _info);
		void Pause();
		
		bool IsPlaying();		
		double GetCurrentRow();
	};
}

#endif //AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_H
