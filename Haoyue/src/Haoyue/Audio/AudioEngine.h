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
		uint32_t NumActiveSounds = 0;		// 当前正在播放的音频数量
		uint32_t TotalSources = 0;			// 音频源总数
		uint64_t MemEngine = 0;				// 音频引擎占用的内存（字节）
		uint64_t MemResManager = 0;			// 资源管理器占用的内存（字节）
		float FrameTime = 0.0f;				// 帧处理时间（毫秒）
		uint64_t NumAudioComps = 0;			// 音频组件数量
	};

	struct AllocationCallbackData
	{
		bool isResourceManager = false;
		Stats& Stats;
	};

    template<typename T>
    struct EntityIDMap
    {
        void Add(Haoyue::UUID sceneID, Haoyue::UUID entityID, T object)
        {
            std::scoped_lock lock{ m_Mutex };
            m_EntityIDMap[sceneID][entityID] = object;
        }

        void Remove(Haoyue::UUID sceneID, Haoyue::UUID entityID)
        {
            std::scoped_lock lock{ m_Mutex };
            HY_CORE_ASSERT(m_EntityIDMap.at(sceneID).find(entityID) != m_EntityIDMap.at(sceneID).end(),
                "Could not find entityID in the registry to remove.");

            m_EntityIDMap.at(sceneID).erase(entityID);
            if (m_EntityIDMap.at(sceneID).empty())
                m_EntityIDMap.erase(sceneID);
        }

        std::optional<T> Get(Haoyue::UUID sceneID, Haoyue::UUID entityID) const
        {
            std::shared_lock lock{ m_Mutex };
            if (m_EntityIDMap.find(sceneID) != m_EntityIDMap.end())
            {
                if (m_EntityIDMap.at(sceneID).find(entityID) != m_EntityIDMap.at(sceneID).end())
                {
                    return std::optional<T>(m_EntityIDMap.at(sceneID).at(entityID));
                }
            }
            return std::optional<T>();
        }

        void Clear(Haoyue::UUID sceneID)
        {
            std::scoped_lock lock{ m_Mutex };
            if (sceneID == 0)
            {
                m_EntityIDMap.clear();
            }
            else if (m_EntityIDMap.find(sceneID) != m_EntityIDMap.end())
            {
                m_EntityIDMap.at(sceneID).clear();
                m_EntityIDMap.erase(sceneID);
            }
        }

        uint64_t Count(Haoyue::UUID sceneID) const
        {
            std::shared_lock lock{ m_Mutex };
            if (m_EntityIDMap.find(sceneID) != m_EntityIDMap.end())
                return m_EntityIDMap.at(sceneID).size();
            else
                return 0;
        }
    private:
        mutable std::shared_mutex m_Mutex;
        std::unordered_map<Haoyue::UUID, std::unordered_map<Haoyue::UUID, T>> m_EntityIDMap;
    };

    struct AudioComponentRegistry : public EntityIDMap<Haoyue::Entity>
    {
        AudioComponent* GetAudioComponent(Haoyue::UUID sceneID, uint64_t audioComponentID) const
        {
            std::optional<Haoyue::Entity> o = Get(sceneID, audioComponentID);
            if (o.has_value())
                return &o->GetComponent<AudioComponent>();
            else
            {
                HY_CORE_ASSERT("Component was not found in registry");
                return nullptr;
            }
        }

        Haoyue::Entity GetEntity(Haoyue::UUID sceneID, uint64_t audioComponentID) const
        {
            return Get(sceneID, Haoyue::UUID(audioComponentID)).value_or(Haoyue::Entity());
        }
	};

	class MiniAudioEngine
	{
	public:
		MiniAudioEngine();
		~MiniAudioEngine();

		static void Init();
		static void Shutdown();

        static inline MiniAudioEngine& Get() { return *s_Instance; }

        static void SetSceneContext(const Haoyue::Ref<Haoyue::Scene>& scene);
        static Haoyue::Ref<Haoyue::Scene>& GetCurrentSceneContext();
        static void OnRuntimePlaying(Haoyue::UUID sceneID);
        static void OnSceneDestruct(Haoyue::UUID sceneID);

        static Stats GetStats();
		bool Initialize();
		bool Uninitialize();

        void StopAll(bool stopNow = false);
        static void StopAllSounds(bool stopNow = false) { s_Instance->StopAll(stopNow); }

        SourceManager& GetSourceManager() { return m_SourceManager; }
        const SourceManager& GetSourceManager() const { return m_SourceManager; }
        void Update(Haoyue::Timestep ts);

        void SubmitSourceUpdateData(std::vector<SoundSourceUpdateData> updateData);
        void UpdateListenerPosition(const glm::vec3& newTranslation, const glm::vec3& newDirection);
        void UpdateListenerVelocity(const glm::vec3& newVelocity);

		static void ExecuteOnAudioThread(AudioThreadCallbackFunction func, const char* jobID = "NONE");

        void RegisterAudioComponent(Haoyue::Entity entity);

        void UnregisterAudioComponent(Haoyue::UUID sceneID, Haoyue::UUID entityID);

	private:
        AudioComponent* GetAudioComponentFromID(Haoyue::UUID sceneID, uint64_t audioComponentID);

		void CreateSources();
		void ReleaseFinishedSources();

        void UpdateSources();
        Sound* GetSoundForAudioComponent(uint64_t audioComponentID);

        Sound* GetSoundForAudioComponent(uint64_t audioComponentID, const SoundConfig& sourceConfig);

        Sound* FreeLowestPrioritySource();


        void SubmitSoundToPlay(uint64_t audioComponentID, const SoundConfig& sourceConfig);
        bool SubmitSoundToPlay(uint64_t audioComponentID);

        bool StopActiveSoundSource(uint64_t audioComponentID);
        bool PauseActiveSoundSource(uint64_t audioComponentID);
        bool IsSoundForComponentPlaying(uint64_t audioComponentID);

	private:
		friend class SourceManager;
		friend class Sound;
        friend class Haoyue::AudioPlayback;

		ma_engine m_Engine;
		bool bInitialized = false;

		SourceManager m_SourceManager{ *this };

		Haoyue::Ref<Haoyue::Scene> m_SceneContext;
		Haoyue::UUID m_CurrentSceneID;

		// 源数限制
		int m_NumSources = 0;
		std::vector<Sound*> m_SoundSources;
        std::vector<SoundObject*> m_ActiveSounds;
		std::vector<SoundObject*> m_SoundsToStart;

        EntityIDMap<Sound*> m_ComponentSoundMap;
        AudioComponentRegistry m_AudioComponentRegistry;

        std::mutex m_UpdateSourcesLock;
        std::vector<SoundSourceUpdateData> m_SourceUpdateData;

		static MiniAudioEngine* s_Instance;

		static Stats s_Stats;
		AllocationCallbackData m_EngineCallbackData{ false, s_Stats };
		AllocationCallbackData m_RMCallbackData{ true, s_Stats };
	};
}

