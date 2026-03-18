#pragma once

namespace Audio {
	class SourceManager
	{
	public:

	private:
		friend class MiniAudioEngine;
		MiniAudioEngine* m_AudioEngine;
	};
}

