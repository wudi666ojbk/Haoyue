#include "pch.h"
#include "Base.h"

#include "Log.h"

namespace Haoyue {

	void InitializeCore()
	{
		Log::Init();

		HY_CORE_TRACE("Initializing...");
	}

	void ShutdownCore()
	{
		HY_CORE_TRACE("Shutting down...");
		
		Log::Shutdown();
	}

}