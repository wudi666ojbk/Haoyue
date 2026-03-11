#pragma once

#ifdef HY_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include <memory>
#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <functional>
#include <algorithm>

#include <fstream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <Haoyue/Core/Base.h>
#include <Haoyue/Core/Log.h>
#include <Haoyue/Core/Events/Event.h>

// Math
#include <Haoyue/Core/Math/Mat4.h>