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
}