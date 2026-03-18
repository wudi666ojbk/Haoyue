#include "pch.h"
#include "Audio.h"

namespace Audio {

    std::thread* AudioThread::s_AudioThread = nullptr;
    std::atomic<bool> AudioThread::s_ThreadActive = false;
    std::atomic<std::thread::id> AudioThread::s_AudioThreadID = std::thread::id();

    std::queue<AudioFunctionCallback*> AudioThread::s_AudioThreadJobs;
    std::mutex AudioThread::s_AudioThreadJobsLock;

    std::function<void(Haoyue::Timestep)> AudioThread::onUpdateCallback = nullptr;
    Haoyue::Timer AudioThread::s_Timer;
    std::atomic<float> AudioThread::s_LastFrameTime = 0.0f;
    Haoyue::Timestep AudioThread::s_TimeStep = 0.0f;

    bool AudioThread::Start()
    {
        if (s_ThreadActive)
            return false;

        s_ThreadActive = true;
        s_AudioThread = new std::thread([]() {
            HRESULT r;
            r = SetThreadDescription(
                GetCurrentThread(),
                L"Hazel Audio Thread"
            );

            HY_CORE_INFO("Spinning up Audio Thread.");
            while (s_ThreadActive)
            {
                OnUpdate();
            }
            HY_CORE_INFO("Audio Thread stopped.");
        });

        s_AudioThreadID = s_AudioThread->get_id();
        s_AudioThread->detach();

        return true;
    }

    bool AudioThread::Stop()
    {
        if (!s_ThreadActive)
            return false;

        s_ThreadActive = false;
        return true;
    }

    bool AudioThread::IsRunning()
    {
        return s_ThreadActive;
    }

    bool AudioThread::IsAudioThread()
    {
        return std::this_thread::get_id() == s_AudioThreadID;
    }

    std::thread::id AudioThread::GetThreadID()
    {
        return s_AudioThreadID;
    }

    void AudioThread::AddTask(AudioFunctionCallback& funCallback)
    {
        std::scoped_lock lock(s_AudioThreadJobsLock);
        s_AudioThreadJobs.emplace(&funCallback);
    }

    void AudioThread::OnUpdate()
    {
        s_Timer.Reset();

        std::scoped_lock lock(s_AudioThreadJobsLock);
        if (!s_AudioThreadJobs.empty())
        {
            // 在作业未完成时不应运行，而是将它们重新添加以在下一个队列中运行
            for (int i = 1 - s_AudioThreadJobs.size(); i >= 0; i--)
            {

            }
        }
    }

}
