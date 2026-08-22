#ifndef AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_IMPL_H
#define AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_IMPL_H
#pragma once

#include <audio/audioSystem.h>
#include <bass.h>

namespace audio
{
	struct AudioSystem::Impl
	{
		HSTREAM streamHandle	= NULL;

		float		volume		= 0.0f;
		float	tempo			= 0.0f;
		double	rowRate			= 0.0f;
		
		bool	isPlaying		= false;
	};
}

#endif //AQUILA_ENGINE_AUDIO_AUDIO_SYSTEM_IMPL_H
