#pragma once

#include <memory>
#include "Ref.h"

namespace Haoyue {

	void InitializeCore();
	void ShutdownCore();

}

#ifndef HY_PLATFORM_WINDOWS
	#error Haoyue only supports Windows!
#endif

// __VA_ARGS__ expansion to get past MSVC "bug"
#define HY_EXPAND_VARGS(x) x

#define BIT(x) (1 << x)

#define HY_BIND_EVENT_FN(fn) std::bind(&##fn, this, std::placeholders::_1)

#include "Assert.h"

// Pointer wrappers
namespace Haoyue {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	using byte = uint8_t;

}