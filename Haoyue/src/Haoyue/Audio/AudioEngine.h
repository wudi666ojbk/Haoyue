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
		uint32_t NumActiveSounds = 0;
		uint32_t TotalSources = 0;
		uint64_t MemEngine = 0;
		uint64_t MemResManager = 0;
		float FrameTime = 0.0f;
		uint64_t NumAudioComps = 0;
	};

	struct AllocationCallbackData // TODO: hide this into Source Manager?
	{
		bool isResourceManager = false;
		Stats& Stats;
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

		bool Initialize();
		bool Uninitialize();

		static MiniAudioEngine& Get() { return *s_Instance; }

		void StopAll(bool stopNow);
		void Update(Haoyue::Timestep ts);

		static Stats GetStats();

		// 在音频线程上执行任意函数。用于同步游戏线程和音频线程之间的更新
		static void ExecuteOnAudioThread(AudioThreadCallbackFunction func, const char* jobID = "NONE");
	private:
		void CreateSources();
		void ReleaseFinishedSources();
	private:
		friend class SourceManager;
		friend class Sound;

		ma_engine m_Engine;
		bool bInitialized = false;

		SourceManager m_SourceManager{ *this };

		AudioListener m_AudioListener;
		Haoyue::Ref<Haoyue::Scene> m_SceneContext;
		Haoyue::UUID m_CurrentSceneID;

		// 源数限制
		int m_NumSources = 0;
		std::vector<Sound*> m_SoundSources;
		std::vector<SoundObject*> m_SoundsToStart;
		std::vector<SoundObject*> m_ActiveSounds;

		static MiniAudioEngine* s_Instance;

		static Stats s_Stats;
		AllocationCallbackData m_EngineCallbackData{ false, s_Stats };
		AllocationCallbackData m_RMCallbackData{ true, s_Stats };
	};
}

