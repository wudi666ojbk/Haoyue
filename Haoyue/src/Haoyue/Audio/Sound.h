#pragma once

#include "SoundObject.h"
#include "miniaudio_incl.h"

#include <glm/glm.hpp>

#include "Haoyue/Asset/Asset.h"

//#include "DSP/FilterAirAbsorption.h"

namespace Audio
{
    class MiniAudioEngine;

    enum class AttenuationModel
    {
        None,          // No distance attenuation and no spatialization.
        Inverse,       // Equivalent to OpenAL's AL_INVERSE_DISTANCE_CLAMPED.
        Linear,        // Linear attenuation. Equivalent to OpenAL's AL_LINEAR_DISTANCE_CLAMPED.
        Exponential    // Exponential attenuation. Equivalent to OpenAL's AL_EXPONENT_DISTANCE_CLAMPED.

        // TODO: CusomCurve
    };

    struct SpatializationConfig : public Haoyue::Asset
    {
        AttenuationModel AttenuationMod{ AttenuationModel::Inverse };   // Distance attenuation function
        float MinGain{ 0.0f };                                            // Minumum volume muliplier
        float MaxGain{ 1.0f };                                            // Maximum volume multiplier
        float MinDistance{ 1.0f };                                        // Distance where to start attenuation
        float MaxDistance{ 1000.0f };                                     // Distance where to end attenuation
        float ConeInnerAngleInRadians{ 6.283185f };                     // Defines the angle where no directional attenuation occurs 
        float ConeOuterAngleInRadians{ 6.283185f };                     // Defines the angle where directional attenuation is at max value (lowest multiplier)
        float ConeOuterGain{ 0.0f };                                      // Attenuation multiplier when direction of the emmiter falls outside of the ConeOuterAngle
        float DopplerFactor{ 1.0f };                                      // The amount of doppler effect to apply. Set to 0 to disables doppler effect. 
        float Rolloff{ 0.6f };                                            // Affects steepness of the attenuation curve. At 1.0 Inverse model is the same as Exponential

        bool bAirAbsorptionEnabled{ true };                            // Enable Air Absorption filter (TODO)

        // TODO
        // float Spread;
        // float Focus;
    };


    /* ==============================================
        Sound configuration to initialize sound source

        Can be passed to an AudioComponent              (TODO: or to directly initialize Sound to play)
        This represents a basic sound to play
        ----------------------------------------------
    */
    struct SoundConfig : public Haoyue::Asset
    {
        Haoyue::Ref<Haoyue::Asset> FileAsset;     // Audio data source

        bool bLooping = false;
        float VolumeMultiplier = 1.0f;
        float PitchMultiplier = 1.0f;

        bool bSpatializationEnabled = false;
        SpatializationConfig Spatialization;    // Configuration for 3D spatialization behavior

        glm::vec3 SpawnLocation{ 0.0f, 0.0f, 0.0f };
    };



    /* =====================================
        Basic Sound, represents playing voice

        -------------------------------------
    */
    class Sound : public SoundObject
    {
    public:
        Sound() = default;
        ~Sound();

        virtual bool Play() override;
        virtual bool Stop() override;
        virtual bool Pause() override;
        virtual bool IsPlaying() const override;

        virtual void SetVolume(float newVolume) override;
        virtual void SetPitch(float newPitch) override;
        void SetLooping(bool looping);

        virtual float GetVolume() override;
        virtual float GetPitch() override;

        virtual bool FadeIn(const float duration, const float targetVolume) override;
        virtual bool FadeOut(const float duration, const float targetVolume) override;

        bool InitializeDataSource(const SoundConfig& soundConfig, MiniAudioEngine* audioEngine);

        void SetLocation(const glm::vec3& location);
        void SetVelocity(const glm::vec3& velocity = { 0.0f, 0.0f, 0.0f });

        bool IsReadyToPlay() const { return bIsReadyToPlay; }

        void Update(Haoyue::Timestep ts) override;
        
        bool IsFinished() const;

        bool IsStopping() const;

        float GetCurrentFadeVolume();

        float GetPriority();

        float GetPlaybackPercentage();

    private:
        bool StopFade(uint64_t numSamples);

        bool StopFade(int milliseconds);

        int StopNow(bool notifyPlaybackComplete = true, bool resetPlaybackPosition = true);

    private:
        friend class MiniAudioEngine;

        std::function<void()> onPlaybackComplete;

        enum class ESoundPlayState
        {
            Stopped,
            Starting,
            Playing,
            Pausing,
            Paused,
            Stopping,
            FadingOut,
            FadingIn
        };
        static const std::string StringFromState(Sound::ESoundPlayState state);

        ESoundPlayState m_PlayState{ ESoundPlayState::Stopped };

        int m_SoundSourceID = -1;

        ma_sound m_Sound;

        bool bIsReadyToPlay = false;

        uint8_t m_Priority = 64;

        bool bLooping = false;
        bool bFinished = false;

        float m_StoredFaderValue = 1.0f;
        float m_LastFadeOutDuration = 0.0f;

        double m_StopFadeTime = 0.0;

        uint64_t m_AudioComponentID = 0;

        uint64_t m_SceneID = 0;
    };
}