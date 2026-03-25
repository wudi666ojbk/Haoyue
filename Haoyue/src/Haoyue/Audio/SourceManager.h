#pragma once

#include "Sound.h"

#include <queue>


namespace Audio
{
    class MiniAudioEngine;

    /* 
    ===========================================================
    音源管理器

    负责音源的生命周期管理、音源参数变更
    以及效果链的构建与更新
    ===========================================================
    */
    class SourceManager
    {
    public:
        SourceManager(MiniAudioEngine& audioEngine);
        ~SourceManager();

        void Initialize();

        bool InitializeSource(unsigned int sourceID, const SoundConfig& sourceConfig);
        void ReleaseSource(unsigned int sourceID);

        bool GetFreeSourceId(int& sourceIdOut);

    private:
        friend class MiniAudioEngine;
        MiniAudioEngine& m_AudioEngine;

        std::queue<int> m_FreeSourcIDs;
    };

}