#include "pch.h"
#include "Sound.h"
#include "AudioEngine.h"
#include "Haoyue/Asset/AssetManager.h"

namespace Audio
{

    const std::string Sound::StringFromState(Sound::ESoundPlayState state)
    {
        std::string str;

        switch (state)
        {
        case ESoundPlayState::Stopped:
            str = "Stopped";
            break;
        case ESoundPlayState::Starting:
            str = "Starting";
            break;
        case ESoundPlayState::Playing:
            str = "Playing";
            break;
        case ESoundPlayState::Pausing:
            str = "Pausing";
            break;
        case ESoundPlayState::Paused:
            str = "Paused";
            break;
        case ESoundPlayState::Stopping:
            str = "Stopping";
            break;
        case ESoundPlayState::FadingOut:
            str = "FadingOut";
            break;
        case ESoundPlayState::FadingIn:
            str = "FadingIn";
            break;
        default:
            break;
        }
        return str;
    }

#if 0 // Enable logging of playback state changes
#define LOG_PLAYBACK(...) HY_CORE_INFO(__VA_ARGS__)
#else
#define LOG_PLAYBACK(...)
#endif

    //==============================================================================

    Sound::~Sound()
    {
        // Need to check if hasn't already been uninitialized by the engine (or has been initialized at all)
        if (m_Sound.engineNode.baseNode.pCachedData != NULL && m_Sound.pDataSource != NULL)
            ma_sound_uninit(&m_Sound);
    }

    bool Sound::InitializeDataSource(const SoundConfig& config, MiniAudioEngine* audioEngine)
    {
        if (bIsReadyToPlay)
        {
            if (IsPlaying())
                ma_sound_stop(&m_Sound);

            if (m_Sound.engineNode.baseNode.pCachedData != NULL || m_Sound.pDataSource != NULL)
                ma_sound_uninit(&m_Sound);

            // TODO: handle reconnection of effects
        }

        // TODO: handle passing in different flags for decoding (from data source asset)
        // TODO: and handle decoding somewhere else, in some other way

        auto& metadata = Haoyue::AssetManager::GetMetadata(config.FileAsset->Handle);

        ma_result result;
        result = ma_sound_init_from_file(&audioEngine->m_Engine, metadata.FilePath.c_str(), MA_SOUND_FLAG_DECODE, nullptr, &m_Sound);

        if (result != MA_SUCCESS)
            return false;

        // TODO: InitializeEffects() and handle spatializers and filters there 

        // TODO: handle using parent's (parent group) spatialization vs override (config probably would be passed down here from parrent)
        const bool isSpatializationEnabled = config.bSpatializationEnabled;

        auto& spatializer = m_Sound.engineNode.spatializer;
        m_Sound.engineNode.isSpatializationDisabled = !isSpatializationEnabled;

        if (isSpatializationEnabled)
        {
            const auto& spatialization = config.Spatialization;
            ma_attenuation_model attMod{ ma_attenuation_model_inverse };

            switch (spatialization.AttenuationMod)
            {
            case AttenuationModel::None:
                attMod = ma_attenuation_model_none;
                break;
            case AttenuationModel::Inverse:
                attMod = ma_attenuation_model_inverse;
                break;
            case AttenuationModel::Linear:
                attMod = ma_attenuation_model_linear;
                break;
            case AttenuationModel::Exponential:
                attMod = ma_attenuation_model_exponential;
                break;
            }

            spatializer.config.attenuationModel = attMod;
            spatializer.config.minGain = spatialization.MinGain;
            spatializer.config.maxGain = spatialization.MaxGain;
            spatializer.config.minDistance = spatialization.MinDistance;
            spatializer.config.maxDistance = spatialization.MaxDistance;
            spatializer.config.coneInnerAngleInRadians = spatialization.ConeInnerAngleInRadians;
            spatializer.config.coneOuterAngleInRadians = spatialization.ConeOuterAngleInRadians;
            spatializer.config.coneOuterGain = spatialization.ConeOuterGain;
            spatializer.config.dopplerFactor = spatialization.DopplerFactor;
            spatializer.config.rolloff = spatialization.Rolloff;

            // If the effect node was initialized previously, no need to do that again
            /*if (spatialization.bAirAbsorptionEnabled)
            {
                if (!bIsReadyToPlay)
                    m_AirAbsorptionFilter.InitNode(m_Sound.engineNode.baseNode.pNodeGraph);

                result = ma_node_attach_output_bus(&m_Sound, 0, m_AirAbsorptionFilter.GetNode(), 0);
            }*/
        }


        SetLooping(config.bLooping);
        SetLocation(config.SpawnLocation);

        bIsReadyToPlay = result == MA_SUCCESS;
        return result == MA_SUCCESS;
    }

    bool Sound::Play()
    {
        if (!IsReadyToPlay())
            return false;

        ma_result result = MA_ERROR;

        switch (m_PlayState)
        {
        case ESoundPlayState::Stopped:
            bFinished = false;
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::Starting:
            break;
        case ESoundPlayState::Playing:
            StopNow(false, true);
            bFinished = false;
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::Pausing:
            StopNow(false, false);
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::Paused:
            // Prepare fade-in for un-pause
            ma_sound_set_fade_in_milliseconds(&m_Sound, 0.0f, m_StoredFaderValue, STOPPING_FADE_MS / 2);
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::Stopping:
            StopNow(true, true);
            result = ma_sound_start(&m_Sound);
            m_PlayState = ESoundPlayState::Starting;
            break;
        case ESoundPlayState::FadingOut:
            break;
        case ESoundPlayState::FadingIn:
            break;
        default:
            break;
        }
        LOG_PLAYBACK(StringFromState(m_PlayState));
        HY_CORE_ASSERT(result == MA_SUCCESS);
        return result == MA_SUCCESS;

        // TODO: consider marking this sound for stop-fade and switching to new one to prevent click on restart
        //       or some other, lower level solution to fade-out stopping while starting to read from the beginning

    }

    bool Sound::Stop()
    {
        bool result = true;
        switch (m_PlayState)
        {
        case ESoundPlayState::Stopped:
            result = false;
            break;
        case ESoundPlayState::Starting:
            StopNow(false, true); // consider stop-fading
            m_PlayState = ESoundPlayState::Stopping;
            break;
        case ESoundPlayState::Playing:
            result = StopFade(STOPPING_FADE_MS);
            m_PlayState = ESoundPlayState::Stopping;
            break;
        case ESoundPlayState::Pausing:
            StopNow(true, true);
            m_PlayState = ESoundPlayState::Stopping;
            break;
        case ESoundPlayState::Paused:
            StopNow(true, true);
            m_PlayState = ESoundPlayState::Stopping;
            break;
        case ESoundPlayState::Stopping:
            StopNow(true, true);
            break;
        case ESoundPlayState::FadingOut:
            break;
        case ESoundPlayState::FadingIn:
            break;
        default:
            break;
        }
        m_LastFadeOutDuration = (float)STOPPING_FADE_MS / 1000.0f;
        LOG_PLAYBACK(StringFromState(m_PlayState));
        return result;
    }

    bool Sound::Pause()
    {
        bool result = true;

        switch (m_PlayState)
        {
        case ESoundPlayState::Playing:
            result = StopFade(STOPPING_FADE_MS);
            m_PlayState = ESoundPlayState::Pausing;
            break;
        case ESoundPlayState::FadingOut:
            break;
        case ESoundPlayState::FadingIn:
            break;
        default:
            result = true;
            break;
        }
        LOG_PLAYBACK(StringFromState(m_PlayState));

        return result;
    }

    bool Sound::StopFade(uint64_t numSamples)
    {
        constexpr double stopFadeTime = (double)STOPPING_FADE_MS * 1.1 / 1000.0;
        m_StopFadeTime = stopFadeTime;

        // Negative volumeBeg starts the fade from the current volume
        ma_result result = ma_sound_set_fade_in_pcm_frames(&m_Sound, -1.0f, 0.0f, numSamples);

        return result == MA_SUCCESS;
    }

    bool Sound::StopFade(int milliseconds)
    {
        HY_CORE_ASSERT(milliseconds > 0);

        const uint64_t fadeInFrames = (milliseconds * m_Sound.engineNode.fader.config.sampleRate) / 1000;

        return StopFade(fadeInFrames);
    }

    bool Sound::FadeIn(const float duration, const float targetVolume)
    {
        /*if (IsPlaying() || bStopping)
            return false;

        ma_result result;
        result = ma_sound_set_fade_in_milliseconds(&m_Sound, 0.0f, targetVolume, uint64_t(duration * 1000));
        if (result != MA_SUCCESS)
            return false;

        bPaused = false;
        m_StoredFaderValue = targetVolume;*/

        return ma_sound_start(&m_Sound) == MA_SUCCESS;
    }

    bool Sound::FadeOut(const float duration, const float targetVolume)
    {
        //if (!IsPlaying())
        //    return false;

        //// If fading out not completely, store the end value to reference later as "current" value
        //if(targetVolume != 0.0f)
        //    m_StoredFaderValue = targetVolume;

        //m_LastFadeOutDuration = duration;
        //bFadingOut = true;
        //StopFade(int(duration * 1000));

        return true;
    }

    bool Sound::IsPlaying() const
    {
        return m_PlayState != ESoundPlayState::Stopped && m_PlayState != ESoundPlayState::Paused;
    }

    bool Sound::IsFinished() const
    {
        return bFinished/* && !bPaused*/;
    }

    bool Sound::IsStopping() const
    {
        return m_PlayState == ESoundPlayState::Stopping;
    }

    void Sound::SetVolume(float newVolume)
    {
        ma_sound_set_volume(&m_Sound, newVolume);
    }

    void Sound::SetPitch(float newPitch)
    {
        ma_sound_set_pitch(&m_Sound, newPitch);
    }

    void Sound::SetLooping(bool looping)
    {
        bLooping = looping;
        ma_sound_set_looping(&m_Sound, bLooping);
    }

    float Sound::GetVolume()
    {
        return ma_node_get_output_bus_volume(&m_Sound, 0);
    }

    float Sound::GetPitch()
    {
        return m_Sound.engineNode.pitch;
    }

    void Sound::SetLocation(const glm::vec3& location)
    {
        ma_sound_set_position(&m_Sound, location.x, location.y, location.z);
        //m_AirAbsorptionFilter.UpdateDistance(glm::distance(location, glm::vec3(0.0f, 0.0f, 0.0f)));
    }

    void Sound::SetVelocity(const glm::vec3& velocity /*= { 0.0f, 0.0f, 0.0f }*/)
    {
        ma_sound_set_velocity(&m_Sound, velocity.x, velocity.y, velocity.z);
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

        auto isFadeFinished = [&]
            {
                return m_StopFadeTime <= 0.0;
            };

        m_StopFadeTime = std::max(0.0, m_StopFadeTime - (double)ts.GetSeconds());

        switch (m_PlayState)
        {
        case ESoundPlayState::Stopped:
            break;
        case ESoundPlayState::Starting:
            if (isNodePlaying())
            {
                m_PlayState = ESoundPlayState::Playing;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState));
            }
            break;
        case ESoundPlayState::Playing:
            if (ma_sound_is_playing(&m_Sound) == MA_FALSE)
            {
                m_PlayState = ESoundPlayState::Stopped;
                bFinished = true;
                notifyIfFinished();
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState));
            }
            break;
        case ESoundPlayState::Pausing:
            if (isFadeFinished())
            {
                StopNow(false, false);
                m_PlayState = ESoundPlayState::Paused;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState));
            }
            break;
        case ESoundPlayState::Paused:
            break;
        case ESoundPlayState::Stopping:
            if (isFadeFinished())
            {
                StopNow(true, true);
                m_PlayState = ESoundPlayState::Stopped;
                LOG_PLAYBACK("Upd: " + StringFromState(m_PlayState));
            }
            break;
        case ESoundPlayState::FadingOut:
            break;
        case ESoundPlayState::FadingIn:
            break;
        default:
            break;
        }
    }

    int Sound::StopNow(bool notifyPlaybackComplete /*= true*/, bool resetPlaybackPosition /*= true*/)
    {
        // Stop reading the data source
        ma_sound_stop(&m_Sound);

        if (resetPlaybackPosition)
        {
            // Reset data source read position to the beginning of the data
            ma_sound_seek_to_pcm_frame(&m_Sound, 0);

            // Mark this voice to be released.
            bFinished = true;
        }
        m_Sound.engineNode.fader.volumeEnd = 1.0f;

        // Need to notify AudioEngine of completion,
        // if this is one shot, AudioComponent needs to be destroyed.
        if (notifyPlaybackComplete && onPlaybackComplete)
            onPlaybackComplete();

        return m_SoundSourceID;
    }

    float Sound::GetCurrentFadeVolume()
    {
        float currentVolume = 0.0f;
        ma_sound_get_current_fade_volume(&m_Sound, &currentVolume);
        // TODO: return volume accounted for distance attenuation, or better read output volume envelope.

        return currentVolume;
    }

    float Sound::GetPriority()
    {
        return GetCurrentFadeVolume() * ((float)m_Priority / 255.0f);
    }

    float Sound::GetPlaybackPercentage()
    {
        ma_uint64 currentFrame;
        ma_uint64 totalFrames;
        ma_sound_get_cursor_in_pcm_frames(&m_Sound, &currentFrame);

        // TODO: concider storing this value when initializing sound;
        ma_sound_get_length_in_pcm_frames(&m_Sound, &totalFrames);


        return (float)currentFrame / (float)totalFrames;
    }
}