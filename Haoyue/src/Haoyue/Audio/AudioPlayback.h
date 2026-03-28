#pragma once

#include "Sound.h"

namespace Haoyue {

	using namespace Audio;

	class AudioPlayback
	{
	public:
		static bool Play(uint64_t audioComponentID, float startTime = 0.0f);
	};
}

