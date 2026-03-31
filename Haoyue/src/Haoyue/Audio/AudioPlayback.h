#pragma once

#include "Sound.h"

namespace Haoyue {

	using namespace Audio;

	class AudioPlayback
	{
	public:
		static bool Play(uint64_t audioComponentID, float startTime = 0.0f);
		static bool StopActiveSound(uint64_t audioComponentID);
		static bool PauseActiveSound(uint64_t audioComponentID);
		static bool IsPlaying(uint64_t audioComponentID);
	};
}

