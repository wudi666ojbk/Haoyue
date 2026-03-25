#include "pch.h"
#include "Sound.h"
#include "AudioEngine.h"

namespace Audio {

    const std::string Sound::StringFromState(Sound::ESoundState state)
    {
        std::string str;
        switch (state)
        {
            case Sound::ESoundState::Stopped:
                str = "Stopped";
                break;
            case Sound::ESoundState::Playing:
                str = "Playing";
                break;
            case Sound::ESoundState::Paused:
                str = "Paused";
                break;
        }
        return str;
    }

    int Sound::StopNow(bool notifyPlaybackComplete, bool resetPlaybackPosition)
    {
        ma_sound_stop(&m_Sound);

        if (resetPlaybackPosition)
        {
            // 将数据源读取位置重置到数据开头
            ma_sound_seek_to_pcm_frame(&m_Sound, 0);

            // 将此音色标记为待释放
            bFinished = true;
        }
        m_Sound.engineNode.fader.volumeEnd = 1.0f;

        // 需要通知音频引擎播放已完成，
        // 如果是一次性播放，则需要销毁音频组件。
        if (notifyPlaybackComplete && onPlaybackComplete)
            onPlaybackComplete();

        return m_SoundSourceID;
    }

    Sound::~Sound()
    {
        if (m_Sound.engineNode.baseNode.pCachedData != NULL && m_Sound.pDataSource != NULL);
            ma_sound_uninit(&m_Sound);
    }

    bool Sound::Play()
    {
        ma_result result = MA_ERROR;
        switch (m_PlayState)
        {
        case ESoundState::Stopped:
            bFinished = false;
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundState::Starting;
            break;
        case ESoundState::Starting:
            break;
        case ESoundState::Playing:
            StopNow(false, true);
            bFinished = false;
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundState::Starting;
            break;
        case ESoundState::Pausing:
            StopNow(false, false);
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundState::Starting;
            break;
        case ESoundState::Paused:
            // 你需要渐入渐出的方法来控制音频的暂停和回复
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundState::Starting;
            break;
        case ESoundState::Stopping:
            StopNow(true, true);
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundState::Starting;
            break;
        default:
            break;
        }
        HY_CORE_ASSERT(result == MA_SUCCESS);
        return result == MA_SUCCESS;
    }

    bool Sound::Stop()
    {
        ma_result result = MA_ERROR;
        return result == MA_SUCCESS;
    }

    bool Sound::Pause()
    {
        return false;
    }
    bool Sound::IsPlaying() const
    {
        return m_PlayState != ESoundState::Stopped && m_PlayState != ESoundState::Paused;
    }

    bool Sound::IsFinished() const
    {
        return bFinished;
    }

    void Sound::Update(Haoyue::Timestep ts)
    {
        auto notifyIfFinished = [&]
        {
            if (ma_sound_at_end(&m_Sound) == MA_TRUE && onPlaybackComplete)
                onPlaybackComplete();
        };

        auto isNodePlaying = [&]
        {
            return ma_sound_is_playing(&m_Sound) == MA_TRUE;
        };

        switch (m_PlayState)
        {
        case ESoundState::Stopped:
            break;
        case ESoundState::Starting:
            if (isNodePlaying())
            {
                m_PlayState = ESoundState::Playing;
            }
            break;
        case ESoundState::Playing:
            if (ma_sound_is_playing(&m_Sound) == MA_FALSE)
            {
                m_PlayState = ESoundState::Stopped;
                bFinished = true;
                notifyIfFinished();
            }
            break;
        case ESoundState::Pausing:
            StopNow(false, false);
            m_PlayState = ESoundState::Paused;
            break;
        case ESoundState::Paused:
            break;
        case ESoundState::Stopping:
            StopNow(true, true);
            m_PlayState = ESoundState::Stopped;
            break;
        default:
            break;
        }
    }

    void Sound::SetVolume(float value)
    {
        ma_sound_set_volume(&m_Sound, value);
    }

    void Sound::SetPitch(float value)
    {
        ma_sound_set_pitch(&m_Sound, value);
    }

    float Sound::GetVolume()
    {
        return 0.0f;
    }

    float Sound::GetPitch()
    {
        return 0.0f;
    }
    bool Sound::InitializeDataSource(const SoundConfig& config, MiniAudioEngine* engine)
    {
        ma_result result;
        result = ma_sound_init_from_file(&engine->m_Engine, config.FileAsset->FilePath.c_str(), MA_SOUND_FLAG_DECODE, NULL, &m_Sound);
        
        if(result != MA_SUCCESS)
            return false;

        return true;
    }
}