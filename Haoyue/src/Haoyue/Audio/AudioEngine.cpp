#include "pch.h"
#include "AudioEngine.h"

namespace Audio {
    
    MiniAudioEngine* MiniAudioEngine::s_Instance = nullptr;

    MiniAudioEngine::MiniAudioEngine()
    {
        AudioThread::BindUpdateFunction([this](Haoyue::Timestep ts) { Update(ts); });
        AudioThread::Start();
        ma_result result;
        ma_engine_config engineConfig = ma_engine_config_init_default();
        engineConfig.periodSizeInFrames = PCM_FRAME_CHUNK_SIZE;

        result = ma_engine_init(&engineConfig, &m_Engine);
    }

    MiniAudioEngine::~MiniAudioEngine()
    {
        ma_engine_uninit(&m_Engine);
    }

    void MiniAudioEngine::Init()
    {
        HY_CORE_ASSERT(s_Instance == nullptr, "音频系统已启动，请勿重复初始化");
        s_Instance = new MiniAudioEngine();

        Sound* sound = new Sound();
        s_Instance->m_SoundSource.push_back(sound);
        SoundConfig config;
        config.m_FileAsset = Haoyue::Ref<Haoyue::Asset>().Create();
        config.m_FileAsset->FilePath = "F:/Project/C++/Engine/Haoyue/Haoyue-Editor/Resources/audio/bigbgm.wav";
        s_Instance->m_SoundSource.at(0)->InitializeDataSource(config, s_Instance);
        s_Instance->m_SoundSource.at(0)->Play();
        
    }

    void MiniAudioEngine::Shutdown()
    {
        HY_CORE_ASSERT(s_Instance != nullptr, "音频系统未启动，请勿关闭");
        // TODO: 关闭正在播放的音频，关闭线程，删除指针
    }

    void MiniAudioEngine::StopAll()
    {
    }

    void MiniAudioEngine::Update(Haoyue::Timestep ts)
    {
        for (auto& sound : m_SoundSource)
        {
            sound->Play();
        }
    }
}