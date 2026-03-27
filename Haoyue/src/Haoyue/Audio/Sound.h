#pragma once
#include "SoundObject.h"
#include "miniaudio_incl.h"

#include "Haoyue/Asset/Asset.h"

#include <glm/glm.hpp>

namespace Audio {

	class MiniAudioEngine;

	enum class AttenuationModel
	{
		None,

	};

	struct SoundConfig : Haoyue::Asset
	{
		Haoyue::Ref<Haoyue::Asset> FileAsset;	// 音频资源路径

		bool bLooping = false;
		float VolumeMultiplier = 1.0f;
		float PitchMultiplier = 1.0f;

		glm::vec3 SpawnLocation{ 0.0f, 0.0f, 0.0f };
	};

	class Sound : public SoundObject
	{
	public:
		~Sound();

		bool Play() override;
		bool Stop() override;
		bool Pause() override;
		bool IsPlaying() const override;
		bool IsFinished() const override;
		bool IsStopping() const;

		void Update(Haoyue::Timestep ts) override;

		void SetVolume(float value) override;
		void SetPitch(float value) override;
		float GetVolume() override;
		float GetPitch() override;

		bool InitializeDataSource(const SoundConfig& config, MiniAudioEngine* engine);
	private:
		friend class MiniAudioEngine;

		std::function<void()> onPlaybackComplete;

		enum class ESoundState
		{
			Stopped,
			Starting,
			Playing,
			Pausing,
			Paused,
			Stopping,
		};
		// 播放状态转字符串
		static const std::string StringFromState(Sound::ESoundState state);

		int StopNow(bool notifyPlaybackComplete = true, bool resetPlaybackPosition = true);
	private:
		ESoundState m_PlayState{ ESoundState::Stopped };

		int m_SoundSourceID = -1;
		ma_sound m_Sound;

		bool bLooping = false;
		bool bFinished = false;

		uint64_t m_AudioComponentID = 0;
		uint64_t m_SceneID = 0;
	};
}

