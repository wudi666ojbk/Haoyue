#include "pch.h"
#include "AudioPlayback.h"

#include "AudioEngine.h"
#include "AudioComponent.h"
#include "Haoyue/Scene/Components.h"

#include "Haoyue/Scene/Scene.h"
#include "Haoyue/Scene/Entity.h"

namespace Haoyue {
    bool AudioPlayback::Play(uint64_t audioComponentID, float startTime)
    {
        auto& engine = MiniAudioEngine::Get();
        return engine.SubmitSoundToPlay(audioComponentID);
    }
    bool AudioPlayback::StopActiveSound(uint64_t audioComponentID)
    {
        auto& engine = MiniAudioEngine::Get();
        return engine.StopActiveSoundSource(audioComponentID);
    }

    bool AudioPlayback::PauseActiveSound(uint64_t audioComponentID)
    {
        auto& engine = MiniAudioEngine::Get();
        return engine.PauseActiveSoundSource(audioComponentID);
    }

    bool AudioPlayback::IsPlaying(uint64_t audioComponentID)
    {
        auto& engine = MiniAudioEngine::Get();
        return engine.IsSoundForComponentPlaying(audioComponentID);
    }
}