#pragma once

#include <thread>
#include <atomic>
#include <queue>

#include "Haoyue/Core/Timer.h"
#include "Haoyue/Core/Timestep.h"

/*----------音频线程管理类----------*/
namespace Audio {
	#define PCM_FRAME_CHUNK_SIZE 1024
	#define STOPPING_FADE_MS 28

	using AudioThreadCallbackFunction = std::function<void()>;
	class AudioFunctionCallback
	{
	public:
		AudioFunctionCallback(AudioThreadCallbackFunction f, const char* jobID = "NONE")
			:m_Func(f), m_JobID(jobID)
		{
		}
		void Execute()
		{
			m_Func();
		}

		const char* GetID() { m_JobID; }
	private:
        AudioThreadCallbackFunction m_Func;
        const char* m_JobID;
	};

	class AudioThread
	{
	public:
		static bool Start();
		static bool Stop();
		static bool IsRunning();
		static bool IsAudioThread();
		static std::thread::id GetThreadID();

	private:
		friend class MiniAudioEngine;

		// 将待执行任务添加到音频线程的任务队列中。
		static void AddTask(AudioFunctionCallback& funCallback);
		static void OnUpdate();

		static float GetFrameTime() { return  s_LastFrameTime.load(); }

		template<typename C, void (C::*F)(Haoyue::Timestep)>
		static void BindUpdateFunction(C* instance)
		{
			onUpdateCallback = [instance](Haoyue::Timestep ts) {(static_cast<C*>(instance)->*Function)(ts); };
		}

		template<typename FuncT>
		static void BindUpdateFunction(FuncT&& func)
		{
			onUpdateCallback = [func](Haoyue::Timestep ts) { func(ts); };
		}
	private:
		static std::thread* s_AudioThread;
		static std::atomic<bool > s_ThreadActive;
		static std::atomic<std::thread::id> s_AudioThreadID;

		static std::queue<AudioFunctionCallback*> s_AudioThreadJobs;
		static std::mutex s_AudioThreadJobsLock;

		static std::function<void(Haoyue::Timestep)> onUpdateCallback;
		static Haoyue::Timer s_Timer;
		static Haoyue::Timestep s_TimeStep;
		static std::atomic<float> s_LastFrameTime;
	};

	static inline bool IsAudioThread()
	{
		return AudioThread::IsAudioThread();
	}
}

