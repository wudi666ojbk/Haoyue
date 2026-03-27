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

		// 用于“一次性音效”。如果设置为 'true'，则播放结束后将销毁所属实体！
		bool bAutoDestroy = false;

		bool bPlayOnAwake = false;

		float VolumeMultiplier = 1.0f;
		float PitchMultiplier = 1.0f;

		glm::vec3 SourcePosition = {1.0f, 1.0f, 1.0f };

		//? 需要自动销毁那些音频组件的声音已播放完毕的实体（“一次性音效”）。
		//? 目前做法：在音频线程中将其标记为待销毁，然后在场景更新时从游戏线程中销毁。
		std::atomic<bool> bMarkedForDestroy = false;

		AudioComponent() = default;

		AudioComponent(const AudioComponent& otheracp)
			: ParentHandle(otheracp.ParentHandle),
			  bAutoDestroy(otheracp.bAutoDestroy),
			  SoundConfig(otheracp.SoundConfig), bPlayOnAwake(otheracp.bPlayOnAwake),
			  VolumeMultiplier(otheracp.VolumeMultiplier), PitchMultiplier(otheracp.PitchMultiplier),
			  SourcePosition(otheracp.SourcePosition)
		{
			bMarkedForDestroy = otheracp.bMarkedForDestroy.load();
		}
		AudioComponent& operator=(const AudioComponent& other)
		{
			ParentHandle = other.ParentHandle;
			SoundConfig = other.SoundConfig;
			bAutoDestroy = other.bAutoDestroy;
			bPlayOnAwake = other.bPlayOnAwake;
			VolumeMultiplier = other.VolumeMultiplier;
			PitchMultiplier = other.PitchMultiplier;
			SourcePosition = other.SourcePosition;
			bMarkedForDestroy = other.bMarkedForDestroy.load();
			return *this;
		}
		explicit AudioComponent(Haoyue::UUID parent)
			: ParentHandle(parent)
		{
		}
	};
}

