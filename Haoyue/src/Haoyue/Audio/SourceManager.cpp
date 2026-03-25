#include "pch.h"
#include "SourceManager.h"
#include "AudioEngine.h"

namespace Audio
{
    SourceManager::SourceManager(MiniAudioEngine& audioEngine)
        : m_AudioEngine(audioEngine)
    {
    }

    SourceManager::~SourceManager()
    {
        // TODO: release all of the sources
    }

    void SourceManager::Initialize()
    {
    }

    bool SourceManager::InitializeSource(unsigned int sourceID, const SoundConfig& sourceConfig)
    {
        return m_AudioEngine.m_SoundSources.at(sourceID)->InitializeDataSource(sourceConfig, &m_AudioEngine);
    }

    void SourceManager::ReleaseSource(unsigned int sourceID)
    {
        m_FreeSourcIDs.push(sourceID);
    }

    bool SourceManager::GetFreeSourceId(int& sourceIdOut)
    {
        if (m_FreeSourcIDs.empty())
            return false;

        sourceIdOut = m_FreeSourcIDs.front();
        m_FreeSourcIDs.pop();

        return true;
    }

}