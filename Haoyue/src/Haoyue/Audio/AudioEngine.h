#pragma once

#include "miniaudio_incl.h"

#include "Haoyue/Core/Timer.h"
#include "Haoyue/Scene/Scene.h"
#include "Haoyue/Scene/Entity.h"

#include "Audio.h"
#include "SourceManager.h"
#include "Sound.h"
#include "AudioComponent.h"
#include "Haoyue/Scene/Components.h"

#include "AudioPlayback.h"

#include <optional>
#include <queue>
#include <shared_mutex>
#include <thread>

#include <glm/glm.hpp>

namespace Audio {

	class SourceManager;
	class Sound;
	class SoundSource;
	class AudioComponent;
	
	struct Stats
	{

	};

	struct AllocationCallbackData
	{
		bool isResourceManager;
		Stats& stats;
	};

	// 音频监听
	struct AudioListener
	{

	};

	class MiniAudioEngine
	{
	public:
		MiniAudioEngine();
		~MiniAudioEngine();

		static void Init();
		static void Shutdown();

		static MiniAudioEngine& Get() { return *s_Instance; }

		void StopAll();
		void Update(Haoyue::Timestep ts);

		static Stats GetStats();
	private:
		void CreateSource();
		void ReleaseSource();
	private:
		friend class SourceManager;
		friend class Sound;

		ma_engine m_Engine;

		SourceManager m_SourceManager{ *this };
		AudioListener m_AudioListener;
		Haoyue::Ref<Haoyue::Scene> m_SceneContext;

		std::vector<Sound*> m_SoundSource;

		static MiniAudioEngine* s_Instance;
	};
}

