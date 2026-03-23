#include "pch.h"
#include "SourceManager.h"

namespace Audio {
    SourceManager::SourceManager(MiniAudioEngine& audioEngine)
        : m_AudioEngine(audioEngine)
    {
    }

    SourceManager::~SourceManager()
    {
    }
}