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

	private:
		void CreateSource();
		void ReleaseSource();

		void Play();

		void StopAll();
		void Stop();
	private:
		ma_engine m_Engine;
		ma_sound m_TestSound;

		static MiniAudioEngine* s_Instance;

		// 场景上下文
		Haoyue::Ref<Haoyue::Scene> m_SceneContext;
	};
}

