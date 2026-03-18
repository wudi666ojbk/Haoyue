#include "pch.h"
#include "AudioEngine.h"

namespace Audio {
    
    MiniAudioEngine* MiniAudioEngine::s_Instance = nullptr;

    MiniAudioEngine::MiniAudioEngine()
    {
        ma_result result;
        ma_engine_config engineConfig = ma_engine_config_init_default();
        engineConfig.periodSizeInFrames = PCM_FRAME_CHUNK_SIZE;

        result = ma_engine_init(&engineConfig, &m_Engine);
        // TODO: 临时代码，用于测试音频播放
        ma_sound_init_from_file(&m_Engine, "Resources/audio/bgm.wav", MA_DATA_SOURCE_FLAG_DECODE, NULL, &m_TestSound);
        ma_sound_set_spatialization_enabled(&m_TestSound, MA_FALSE);
        ma_sound_start(&m_TestSound);
        ma_sound_set_fade_in_milliseconds(&m_TestSound, 0.0f, 1.0f, 200);
    }

    MiniAudioEngine::~MiniAudioEngine()
    {
        ma_engine_uninit(&m_Engine);
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
}