#include "pch.h"
#include "AudioEngine.h"

namespace Audio {
    
    MiniAudioEngine* MiniAudioEngine::s_Instance = nullptr;
    Stats MiniAudioEngine::s_Stats;

    void MiniAudioEngine::ExecuteOnAudioThread(AudioThreadCallbackFunction func, const char* jobID/* = "NONE"*/)
    {
        AudioThread::AddTask(new AudioFunctionCallback(std::move(func), jobID));
    }

    void MemFreeCallback(void* p, void* pUserData)
    {
        if (p == NULL)
            return;

        constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

        char* buffer = (char*)p - offset; //get the start of the buffer
        int* sizeBox = (int*)buffer;

        auto* alData = (AllocationCallbackData*)pUserData;
        if (alData->isResourceManager) alData->Stats.MemResManager -= *sizeBox;
        else                           alData->Stats.MemEngine -= *sizeBox;

        std::free(buffer);
    }
    void* MemAllocCallback(size_t sz, void* pUserData)
    {
        constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

        char* buffer = (char*)malloc(sz + offset); //allocate offset extra bytes 
        if (buffer == NULL)
            return NULL; // no memory! 

        auto* alData = (AllocationCallbackData*)pUserData;
        if (alData->isResourceManager) alData->Stats.MemResManager += sz;
        else                           alData->Stats.MemEngine += sz;

        int* sizeBox = (int*)buffer;
        *sizeBox = sz; //store the size in first sizeof(int) bytes!
        HY_CORE_INFO("Allocated mem KB: {0}", *sizeBox / 1000.0f);
        return buffer + offset; //return buffer after offset bytes!
    }
    void* MemReallocCallback(void* p, size_t sz, void* pUserData)
    {
        constexpr auto offset = std::max(sizeof(int), alignof(max_align_t));

        auto* buffer = (char*)p - offset; //get the start of the buffer
        int* sizeBox = (int*)buffer;

        auto* alData = (AllocationCallbackData*)pUserData;
        if (alData->isResourceManager) alData->Stats.MemResManager += sz - *sizeBox;
        else                           alData->Stats.MemEngine += sz - *sizeBox;

        *sizeBox = sz;

        auto* newBuffer = (char*)ma_realloc(buffer, sz, NULL);
        if (newBuffer == NULL)
            return NULL;

        HY_CORE_INFO("Reallocated mem KB: {0}", sz / 1000.0f);
        return newBuffer + offset;
    }

    void MALogCallback(ma_context* pContext, ma_device* pDevice, ma_uint32 logLevel, const char* message)
    {
        HY_CORE_INFO(message);
    }

    //==========================================================================
    MiniAudioEngine::MiniAudioEngine()
    {
        AudioThread::BindUpdateFunction([this](Haoyue::Timestep ts) { Update(ts); });
        AudioThread::Start();

        MiniAudioEngine::ExecuteOnAudioThread([this] { Initialize(); }, "InitializeAudioEngine");
    }

    MiniAudioEngine::~MiniAudioEngine()
    {
        if (bInitialized)
            Uninitialize();
    }

    void MiniAudioEngine::Init()
    {
        HY_CORE_ASSERT(s_Instance == nullptr, "音频系统已启动，请勿重复初始化");
        s_Instance = new MiniAudioEngine();
    }

    void MiniAudioEngine::Shutdown()
    {
        HY_CORE_ASSERT(s_Instance != nullptr, "音频系统未启动，请勿关闭");
        s_Instance->Uninitialize();
        delete s_Instance;
        s_Instance = nullptr;
    }

    bool MiniAudioEngine::Initialize()
    {
        if (bInitialized)
            return true;

        ma_result result;
        ma_engine_config engineConfig = ma_engine_config_init_default();
        engineConfig.periodSizeInFrames = PCM_FRAME_CHUNK_SIZE;
        ma_allocation_callbacks allocationCallbacks{ &m_EngineCallbackData, &MemAllocCallback, &MemReallocCallback, &MemFreeCallback };
        engineConfig.allocationCallbacks = allocationCallbacks;

        result = ma_engine_init(&engineConfig, &m_Engine);
        if (result != MA_SUCCESS)
        {
            HY_CORE_ASSERT(false, "Failed to initialize audio engine.");
            return false;
        }
        m_Engine.pDevice->pContext->logCallback = &MALogCallback;

        allocationCallbacks.pUserData = &m_RMCallbackData;
        m_Engine.pResourceManager->config.allocationCallbacks = allocationCallbacks;
        m_NumSources = 32;
        CreateSources();

        s_Stats.TotalSources = m_NumSources;

        bInitialized = true;
        return true;
    }
    bool MiniAudioEngine::Uninitialize()
    {
        StopAll(true);

        m_SceneContext = nullptr;
        m_ComponentSoundMap.Clear(0);
        m_AudioComponentRegistry.Clear(0);

        AudioThread::Stop();

        ma_engine_uninit(&m_Engine);
        
        for (auto* sound : m_SoundSources)
            delete sound;

        HY_CORE_INFO("音频模块已被卸载");
        bInitialized = false;

        return true;
    }

    void MiniAudioEngine::CreateSources()
    {
        m_SoundSources.reserve(m_NumSources);
        for (int i = 0; i < m_NumSources; i++)
        {
            Sound* soundSource = new Sound();
            soundSource->m_SoundSourceID = i;
            m_SoundSources.push_back(soundSource);
            m_SourceManager.m_FreeSourcIDs.push(i);
        }
    }

    Sound* MiniAudioEngine::FreeLowestPrioritySource()
    {
        HY_CORE_ASSERT(m_SourceManager.m_FreeSourcIDs.empty());

        Sound* lowestPriStoppingSource = nullptr;
        Sound* lowestPriNonLoopingSource = nullptr;
        Sound* lowestPriSource = nullptr;

        // Compare and return lower priority Source of the two
        auto getLowerPriority = [](Sound* sourceToCheck, Sound* sourceLowest, bool checkPlaybackPos = false) -> Sound*
            {
                if (!sourceLowest)
                {
                    return sourceToCheck;
                }
                else
                {
                    float a = sourceToCheck->GetPriority();
                    float b = sourceLowest->GetPriority();

                    if (a < b)             return sourceToCheck;
                    else if (a > b)             return sourceLowest;
                    else if (checkPlaybackPos)  return sourceToCheck->GetPlaybackPercentage() > sourceLowest->GetPlaybackPercentage() ? sourceToCheck : sourceLowest;
                    else                        return sourceLowest;
                }
            };

        for (SoundObject* so : m_ActiveSounds)
        {
            if (auto* source = dynamic_cast<Sound*> (so))
            {
                if (source->IsStopping())
                {
                    lowestPriStoppingSource = getLowerPriority(source, lowestPriStoppingSource);
                }
                else
                {
                    if (!source->bLooping)
                    {
                        // Checking playback percentage here, in case the volume weighted prioprity is the same
                        lowestPriNonLoopingSource = getLowerPriority(source, lowestPriNonLoopingSource, true);
                    }
                    else
                    {
                        lowestPriSource = getLowerPriority(source, lowestPriSource);
                    }
                }
            }
        }

        Sound* releasedSoundSource = nullptr;

        if (lowestPriStoppingSource)   releasedSoundSource = lowestPriNonLoopingSource;
        else if (lowestPriNonLoopingSource) releasedSoundSource = lowestPriNonLoopingSource;
        else                                releasedSoundSource = lowestPriSource;

        HY_CORE_ASSERT(releasedSoundSource);

        releasedSoundSource->StopNow();
        ReleaseFinishedSources();

        HY_CORE_ASSERT(!m_SourceManager.m_FreeSourcIDs.empty());

        return releasedSoundSource;
    }

    Sound* Audio::MiniAudioEngine::GetSoundForAudioComponent(uint64_t audioComponentID)
    {
        auto sceneID = s_Instance->m_CurrentSceneID;
        return m_ComponentSoundMap.Get(sceneID, audioComponentID).value_or(nullptr);
    }

    Sound* MiniAudioEngine::GetSoundForAudioComponent(uint64_t audioComponentID, const SoundConfig& sourceConfig)
    {
        HY_CORE_ASSERT(AudioThread::IsAudioThread());
        auto sceneID = s_Instance->m_CurrentSceneID;

        // If component already has an active sound, return reference to the active sound
        if (auto* s = m_ComponentSoundMap.Get(sceneID, audioComponentID).value_or(nullptr))
            return s;

        // TODO: For now just handling basic Sounds, more complex SoundObjects later
        Sound* sound = nullptr;

        int freeID;
        if (!m_SourceManager.GetFreeSourceId(freeID))
        {
            // Stop lowest priority source
            FreeLowestPrioritySource();
            m_SourceManager.GetFreeSourceId(freeID);
        }

        sound = m_SoundSources.at(freeID);

        sound->m_AudioComponentID = audioComponentID;

        sound->m_SceneID = sceneID;

        m_ComponentSoundMap.Add(sceneID, audioComponentID, sound);

        if (!m_SourceManager.InitializeSource(freeID, sourceConfig))
        {
            return nullptr;
        }

        m_ActiveSounds.push_back(sound);

        return sound;
    }


    //==================================================================================

    void MiniAudioEngine::SubmitSoundToPlay(uint64_t audioComponentID, const SoundConfig& sourceConfig)
    {
        auto startSound = [this, audioComponentID, sourceConfig]
            {
                if (Sound* sound = GetSoundForAudioComponent(audioComponentID, sourceConfig))
                {
                    sound->onPlaybackComplete = [this, audioComponentID]
                        {
                            if (auto* ac = GetAudioComponentFromID(m_CurrentSceneID, audioComponentID))
                                ac->bMarkedForDestroy = true;
                        };

                    sound->SetVolume(sourceConfig.VolumeMultiplier);
                    sound->SetPitch(sourceConfig.PitchMultiplier);
                    m_SoundsToStart.push_back(sound);
                }
            };
        AudioThread::IsAudioThread() ? startSound() : ExecuteOnAudioThread(startSound, "StartSound");
    }

    bool MiniAudioEngine::SubmitSoundToPlay(uint64_t audioComponentID)
    {
        auto* ac = GetAudioComponentFromID(m_CurrentSceneID, audioComponentID);
        if (ac == nullptr)
        {
            HY_CORE_ASSERT(ac, "AudioComponent was not found in registry!");
            return false;
        }

        ac->VolumeMultiplier = ac->SoundConfig.VolumeMultiplier;
        ac->PitchMultiplier = ac->SoundConfig.PitchMultiplier;
        SubmitSoundToPlay(audioComponentID, ac->SoundConfig);

        return true;
    }

    bool MiniAudioEngine::StopActiveSoundSource(uint64_t audioComponentID)
    {
        if (!m_ComponentSoundMap.Get(m_CurrentSceneID, audioComponentID).has_value())
            return false;

        auto stopSound = [this, audioComponentID]
            {
                if (auto* sound = m_ComponentSoundMap.Get(m_CurrentSceneID, audioComponentID).value_or(nullptr))
                    sound->Stop();
            };

        AudioThread::IsAudioThread() ? stopSound() : ExecuteOnAudioThread(stopSound, "StopSound");

        return true;
    }

    bool MiniAudioEngine::PauseActiveSoundSource(uint64_t audioComponentID)
    {
        if (!m_ComponentSoundMap.Get(m_CurrentSceneID, audioComponentID).has_value())
            return false;

        auto pauseSound = [this, audioComponentID]
            {
                if (auto* sound = m_ComponentSoundMap.Get(m_CurrentSceneID, audioComponentID).value_or(nullptr))
                    sound->Pause();
            };

        AudioThread::IsAudioThread() ? pauseSound() : ExecuteOnAudioThread(pauseSound, "PauseSound");

        return true;
    }

    bool MiniAudioEngine::IsSoundForComponentPlaying(uint64_t audioComponentID)
    {
        return m_ComponentSoundMap.Get(m_CurrentSceneID, audioComponentID).has_value();
    }


    //==================================================================================

    void MiniAudioEngine::UpdateListener()
    {
        if (m_AudioListener.HasChanged(true))
        {
            glm::vec3 pos, dir, vel;
            m_AudioListener.GetPositionDirection(pos, dir);
            m_AudioListener.GetVelocity(vel);
            ma_engine_listener_set_position(&m_Engine, 0, pos.x, pos.y, pos.z);
            ma_engine_listener_set_direction(&m_Engine, 0, dir.x, dir.y, dir.z);
            ma_engine_listener_set_velocity(&m_Engine, 0, vel.x, vel.y, vel.z);
            // TODO
            //ma_engine_listener_set_cone(&m_Engine, 0);
        }
    }

    void MiniAudioEngine::UpdateSources()
    {
        
        std::scoped_lock lock{ m_UpdateSourcesLock };
        for (auto& data : m_SourceUpdateData)
        {
            if (auto* sound = GetSoundForAudioComponent(data.entityID))
            {
                sound->SetVolume(data.VolumeMultiplier);
                sound->SetPitch(data.PitchMultiplier);
                sound->SetLocation(data.Position);
                sound->SetVelocity(data.Velocity);
            }
        }
    }

    void MiniAudioEngine::SubmitSourceUpdateData(std::vector<SoundSourceUpdateData> updateData)
    {
        std::scoped_lock lock{ m_UpdateSourcesLock };
        m_SourceUpdateData.swap(updateData);
    }

    void MiniAudioEngine::UpdateListenerPosition(const glm::vec3& newTranslation, const glm::vec3& newDirection)
    {
        if (m_AudioListener.PositionNeedsUpdate(newTranslation, newDirection))
            m_AudioListener.SetNewPositionDirection(newTranslation, newDirection);
    }

    void MiniAudioEngine::UpdateListenerVelocity(const glm::vec3& newVelocity)
    {
        m_AudioListener.SetNewVelocity(newVelocity);
    }

    void MiniAudioEngine::ReleaseFinishedSources()
    {
        for (int i = m_ActiveSounds.size() - 1; i >= 0; i--)
        {
            if (Sound* source = dynamic_cast<Sound*>(m_ActiveSounds[i]))
            {
                if (source->IsFinished())
                {
                    m_ActiveSounds.erase(m_ActiveSounds.begin() + i);
                    m_ComponentSoundMap.Remove(source->m_SceneID, source->m_AudioComponentID);

                    m_SourceManager.m_FreeSourcIDs.push(source->m_SoundSourceID);
                    s_Stats.NumActiveSounds = m_ActiveSounds.size();
                }
            }
        }
    }

    void MiniAudioEngine::Update(Haoyue::Timestep ts)
    {
        UpdateSources();
        for (auto* sound : m_SoundsToStart)
        {
            sound->Play();
            s_Stats.NumActiveSounds = m_ActiveSounds.size();
        }
        m_SoundsToStart.clear();

        for (auto* sound : m_ActiveSounds)
            sound->Update(ts);

        ReleaseFinishedSources();
    }

    Stats MiniAudioEngine::GetStats()
    {
        s_Stats.FrameTime = AudioThread::GetFrameTime();
        return MiniAudioEngine::s_Stats;
    }

    void MiniAudioEngine::StopAll(bool stopNow)
    {
        auto stopAll = [&, stopNow]
            {
                m_SoundsToStart.clear();

                for (auto* sound : m_ActiveSounds)
                {
                    if (stopNow)
                        dynamic_cast<Sound*>(sound)->StopNow(true, true);
                    else
                        sound->Stop();
                }
            };

        AudioThread::IsAudioThread() ? stopAll() : ExecuteOnAudioThread(stopAll, "StopAll");
    }

    void MiniAudioEngine::SetSceneContext(const Haoyue::Ref<Haoyue::Scene>& scene)
    {
        auto& audioEngine = Get();

        audioEngine.StopAll();

        audioEngine.m_SceneContext = scene;
        auto* newScene = audioEngine.m_SceneContext.Raw();
        const auto newSceneID = newScene->GetUUID();
        audioEngine.m_CurrentSceneID = newSceneID;

        s_Stats.NumAudioComps = s_Instance->m_AudioComponentRegistry.Count(newSceneID);

        auto view = newScene->GetAllEntitiesWith<Audio::AudioComponent>();
        for (auto entity : view)
        {
            Haoyue::Entity e = { entity, newScene };
            auto& audioComp = e.GetComponent<Audio::AudioComponent>();

            s_Instance->RegisterAudioComponent(e);
        }
    }

    void Audio::MiniAudioEngine::OnRuntimePlaying(Haoyue::UUID sceneID)
    {
        auto& audioEngine = Get();

        const auto currentSceneID = audioEngine.m_CurrentSceneID;
        HY_CORE_ASSERT(currentSceneID == sceneID)

            auto newScene = Haoyue::Scene::GetScene(currentSceneID);

        auto view = newScene->GetAllEntitiesWith<Audio::AudioComponent>();
        for (auto entity : view)
        {
            Haoyue::Entity audioEntity = { entity, newScene.Raw() };

            auto& ac = audioEntity.GetComponent<Audio::AudioComponent>();
            if (ac.bPlayOnAwake)
            {
                auto newScene = Haoyue::Scene::GetScene(currentSceneID);
                if (!newScene->IsEditorScene() && newScene->IsPlaying())
                {
                    auto translation = audioEntity.GetComponent<Haoyue::TransformComponent>().Translation;
                    ac.SourcePosition = translation;
                    ac.SoundConfig.SpawnLocation = translation;
                    audioEngine.SubmitSoundToPlay(audioEntity.GetUUID());
                }
            }
        }
    }

    void MiniAudioEngine::OnSceneDestruct(Haoyue::UUID sceneID)
    {
        Get().m_AudioComponentRegistry.Clear(sceneID);
    }

    Haoyue::Ref<Haoyue::Scene>& MiniAudioEngine::GetCurrentSceneContext()
    {
        return Get().m_SceneContext;
    }


    //==================================================================================

    AudioComponent* Audio::MiniAudioEngine::GetAudioComponentFromID(Haoyue::UUID sceneID, uint64_t audioComponentID)
    {
        return m_AudioComponentRegistry.GetAudioComponent(sceneID, audioComponentID);
    }

    void MiniAudioEngine::RegisterAudioComponent(Haoyue::Entity audioEntity)
    {
        const auto sceneID = audioEntity.GetSceneUUID();
        m_AudioComponentRegistry.Add(sceneID, audioEntity.GetUUID(), audioEntity);
        s_Stats.NumAudioComps = m_AudioComponentRegistry.Count(sceneID);

        uint64_t entityID = audioEntity.GetUUID();
        auto* ac = GetAudioComponentFromID(sceneID, entityID);
        if (ac->bPlayOnAwake)
        {
            auto newScene = Haoyue::Scene::GetScene(sceneID);
            if (!newScene->IsEditorScene() && newScene->IsPlaying())
            {
                auto translation = audioEntity.GetComponent<Haoyue::TransformComponent>().Translation;
                ac->SourcePosition = translation;
                ac->SoundConfig.SpawnLocation = translation;
                SubmitSoundToPlay(entityID);
            }
        }
    }

    void MiniAudioEngine::UnregisterAudioComponent(Haoyue::UUID sceneID, Haoyue::UUID entityID)
    {
        m_AudioComponentRegistry.Remove(sceneID, entityID);
        s_Stats.NumAudioComps = m_AudioComponentRegistry.Count(sceneID);
    }
}