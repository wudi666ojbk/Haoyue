#pragma once

#define HY_ENABLE_PROFILING !HY_DIST

#if HY_ENABLE_PROFILING 
#include <tracy/Tracy.hpp>
#endif

#if HY_ENABLE_PROFILING
#define HY_PROFILE_MARK_FRAME			FrameMark;
#define HY_PROFILE_FUNC(...)			ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#define HY_PROFILE_SCOPE(...)			HY_PROFILE_FUNC(__VA_ARGS__)
#define HY_PROFILE_SCOPE_DYNAMIC(NAME)  ZoneScoped; ZoneName(NAME, strlen(NAME))
#define HY_PROFILE_THREAD(...)          tracy::SetThreadName(__VA_ARGS__)
#else
#define HY_PROFILE_MARK_FRAME
#define HY_PROFILE_FUNC(...)
#define HY_PROFILE_SCOPE(...)
#define HY_PROFILE_SCOPE_DYNAMIC(NAME)
#define HY_PROFILE_THREAD(...)
#endif
