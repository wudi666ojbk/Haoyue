#pragma once

#include "Haoyue/Core/TimeStep.h"

namespace Audio {

	class SoundObject
	{
	public:
		virtual ~SoundObject() = default;

        virtual bool Play() = 0;
        virtual bool Stop() = 0;
        virtual bool Pause() = 0;
		virtual bool IsPlaying() = 0;

		virtual void SetVolume(float value) = 0;
		virtual void SetPitch(float value) = 0;
		virtual float GetVolume() = 0;
		virtual float GetPitch() = 0;
	};
}

