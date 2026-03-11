#pragma once

#include "Haoyue/Core/Base.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#include <glm/glm.hpp>

namespace Haoyue {

	class Log
	{
	public:
		static void Init();
		static void Shutdown();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

template<typename OStream>
OStream& operator<<(OStream& os, const glm::vec3& vec)
{
	return os << '(' << vec.x << ", " << vec.y << ", " << vec.z << ')';
}

template<typename OStream>
OStream& operator<<(OStream& os, const glm::vec4& vec)
{
	return os << '(' << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ')';
}

// Core Logging Macros
#define HY_CORE_TRACE(...)	Haoyue::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define HY_CORE_INFO(...)	Haoyue::Log::GetCoreLogger()->info(__VA_ARGS__)
#define HY_CORE_WARN(...)	Haoyue::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define HY_CORE_ERROR(...)	Haoyue::Log::GetCoreLogger()->error(__VA_ARGS__)
#define HY_CORE_FATAL(...)	Haoyue::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client Logging Macros
#define HY_TRACE(...)	Haoyue::Log::GetClientLogger()->trace(__VA_ARGS__)
#define HY_INFO(...)	Haoyue::Log::GetClientLogger()->info(__VA_ARGS__)
#define HY_WARN(...)	Haoyue::Log::GetClientLogger()->warn(__VA_ARGS__)
#define HY_ERROR(...)	Haoyue::Log::GetClientLogger()->error(__VA_ARGS__)
#define HY_FATAL(...)	Haoyue::Log::GetClientLogger()->critical(__VA_ARGS__)