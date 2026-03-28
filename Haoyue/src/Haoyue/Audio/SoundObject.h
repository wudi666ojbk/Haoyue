#pragma once

#include "Haoyue/Core/TimeStep.h"


namespace Audio
{
    class SoundObject
    {
    public:
        virtual ~SoundObject() = default;

        virtual bool Play() = 0;
        virtual bool Stop() = 0;
        virtual bool Pause() = 0;
        virtual bool IsPlaying() const = 0;

        virtual void SetVolume(float newVolume) = 0;
        virtual void SetPitch(float newPitch) = 0;

        virtual float GetVolume() = 0;
        virtual float GetPitch() = 0;

        virtual bool FadeIn(const float duration, const float targetVolume) = 0;
        virtual bool FadeOut(const float duration, const float targetVolume) = 0;

        virtual void Update(Haoyue::Timestep ts) = 0;
    };

}