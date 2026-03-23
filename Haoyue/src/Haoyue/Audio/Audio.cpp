#include "pch.h"
#include "Audio.h"

namespace Audio
{
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
        s_AudioThread = new std::thread([]
            {

#if defined(HY_PLATFORM_WINDOWS)

                HRESULT r;
                r = SetThreadDescription(
                    GetCurrentThread(),
                    L"Haoyue Audio Thread"
                );
#endif

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

    void AudioThread::AddTask(AudioFunctionCallback*&& funcCb)
    {
        std::scoped_lock lock(s_AudioThreadJobsLock);
        s_AudioThreadJobs.emplace(std::move(funcCb));
    }

    void AudioThread::OnUpdate()
    {
        s_Timer.Reset();

        //---------------------------
        //--- Handle AudioThread Jobs

        std::scoped_lock lock(s_AudioThreadJobsLock);
        if (!s_AudioThreadJobs.empty())
        {
            // Should not run while jobs are not complete, instead re-add them to run on the next update loop
            //while (!s_AudioThreadJobs.empty())
            for (int i = s_AudioThreadJobs.size() - 1; i >= 0; i--)
            {
                auto job = s_AudioThreadJobs.front();

                job->Execute();

                // TODO: check if job ran successfully, if not, notify and/or add back to the queue
                s_AudioThreadJobs.pop();
                delete job;
            }
        }

        HY_CORE_ASSERT(onUpdateCallback != nullptr, "Update Function is not bound!");
        onUpdateCallback(s_TimeStep);

        s_TimeStep = s_Timer.Elapsed();
        s_LastFrameTime = s_TimeStep.GetMilliseconds();
    }

} // namespace Audio