#include "pch.h"
#include "AudioEngine.h"

namespace Audio {
    
    MiniAudioEngine* MiniAudioEngine::s_Instance = nullptr;
    Stats MiniAudioEngine::s_Stats;

    void MiniAudioEngine::ExecuteOnAudioThread(AudioThreadCallbackFunction func, const char* jobID/* = "NONE"*/)
    {
        AudioThread::AddTask(new AudioFunctionCallback(std::move(func), jobID));
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

        HY_CORE_INFO("Freed mem KB: {0}", *sizeBox / 1000.0f);
        std::free(buffer);
    }

/*-----------------------------------------------------------------------
-------------------------------AudioEngine-------------------------------
------------------------------------------------------------------------*/
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
        // TODO: 关闭正在播放的音频，关闭线程，删除指针
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
        allocationCallbacks.pUserData = &m_RMCallbackData;
        m_Engine.pResourceManager->config.allocationCallbacks = allocationCallbacks;
        m_NumSources = 32;
        CreateSources();

        HY_CORE_INFO(R"(Audio Engine: engine initialized.
                    -----------------------------
                    Endpoint Device Name:   {0}
                    Sample Rate:            {1}
                    Channels:               {2}
                    -----------------------------
                    Callback Buffer Size:   {3}
                    Number of Sources:      {4}
                    -----------------------------)",
            m_Engine.pDevice->playback.name,
            m_Engine.pDevice->sampleRate,
            m_Engine.pDevice->playback.channels,
            engineConfig.periodSizeInFrames,
            m_NumSources);

        s_Stats.TotalSources = m_NumSources;

        bInitialized = true;
        return true;
    }
    bool MiniAudioEngine::Uninitialize()
    {
        StopAll(true);
        
        AudioThread::Stop();

        ma_engine_uninit(&m_Engine);
        
        for (auto* sound : m_SoundSources)
            delete sound;

        HY_CORE_INFO("音频模块已被卸载");
        bInitialized = false;
        return true;
    }

    void MiniAudioEngine::StopAll(bool stopNow)
    {
        auto stopAll = [&, stopNow]
            {
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

    void MiniAudioEngine::Update(Haoyue::Timestep ts)
    {
        for (auto& sound : m_SoundsToStart)
        {
            sound->Play();
            s_Stats.NumActiveSounds = m_ActiveSounds.size();
        }
        m_SoundSources.clear();
    }

    void MiniAudioEngine::CreateSources()
    {
        m_SoundSources.reserve(m_NumSources);
        for (int i = 0; i < m_NumSources; i++)
        {
            Sound* soundSource = new Sound();
            soundSource->m_SoundSourceID = i;
            m_SoundSources.push_back(soundSource);
        }
    }
    void MiniAudioEngine::ReleaseSources()
    {
    }
}