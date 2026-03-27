#pragma once

#include "Sound.h"

#include <queue>


namespace Audio
{
    class MiniAudioEngine;

    /* ===========================================================
        Sound Source Manager

        Handles lifetime of sound sources, source parameter changes
        Building and updating effect chains
        -----------------------------------------------------------
    */
    class SourceManager
    {
    public:
        SourceManager(MiniAudioEngine& audioEngine);

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