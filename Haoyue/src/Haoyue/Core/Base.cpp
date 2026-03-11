#include "pch.h"
#include "Base.h"

#include "Log.h"

#define HAZEL_BUILD_ID "v0.1a"

namespace Haoyue {

	void InitializeCore()
	{
		Log::Init();

		HY_CORE_TRACE("Haoyue Engine {}", HAZEL_BUILD_ID);
		HY_CORE_TRACE("Initializing...");
	}

	void ShutdownCore()
	{
		HY_CORE_TRACE("Shutting down...");
		
		Log::Shutdown();
	}

}