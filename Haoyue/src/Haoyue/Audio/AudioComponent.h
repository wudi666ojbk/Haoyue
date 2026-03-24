#pragma once

#include "Sound.h"

namespace Audio {

	struct SoundSourceUpdateData
	{
		uint64_t entityID;
		float VolumeMultiplier;
		float PitchMultiplier;

		glm::vec3 Position;
        glm::vec3 Velocity;
	};

	struct AudioComponent
	{
		Haoyue::UUID ParentHandle;

		SoundConfig SoundConfig;

		bool bPlayOnAwake = false;

		float VolumeMultiplier = 1.0f;
		float PitchMultiplier = 1.0f;

		glm::vec3 SourcePosition = {1.0f, 1.0f, 1.0f };

		AudioComponent() = default;

		AudioComponent(const AudioComponent& otherCP)
			: ParentHandle(otherCP.ParentHandle),
			  SoundConfig(otherCP.SoundConfig), bPlayOnAwake(otherCP.bPlayOnAwake),
			  VolumeMultiplier(otherCP.VolumeMultiplier), PitchMultiplier(otherCP.PitchMultiplier),
			  SourcePosition(otherCP.SourcePosition)
		{
		}
		AudioComponent& operator=(const AudioComponent& other)
		{
			ParentHandle = other.ParentHandle;
			SoundConfig = other.SoundConfig;
			bPlayOnAwake = other.bPlayOnAwake;
			VolumeMultiplier = other.VolumeMultiplier;
			PitchMultiplier = other.PitchMultiplier;
			SourcePosition = other.SourcePosition;
			return *this;
		}
		explicit AudioComponent(Haoyue::UUID parent)
			: ParentHandle(parent)
		{
		}
	};
}

