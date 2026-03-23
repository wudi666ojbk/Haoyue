#pragma once

#include "Sound.h"

namespace Audio {

	class MiniAudioEngine;

	class SourceManager
	{
	public:
		SourceManager(MiniAudioEngine& audioEngine);
		~SourceManager();

	private:
		friend class MiniAudioEngine;
		MiniAudioEngine& m_AudioEngine;
	};
}

