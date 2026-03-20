#pragma once

#include <spdlog/spdlog.h>

namespace Vanta {

    class Log
    {
    public:
        static void Init();
        static void Shutdown();

        static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_coreLogger; }
        static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_clientLogger; }
    private:
        static std::shared_ptr<spdlog::logger> s_coreLogger;
        static std::shared_ptr<spdlog::logger> s_clientLogger;
    };

}

// Core
#define VA_CORE_TRACE(...)   ::Vanta::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VA_CORE_INFO(...)    ::Vanta::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VA_CORE_WARN(...)    ::Vanta::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VA_CORE_ERROR(...)   ::Vanta::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VA_CORE_CRITICAL(...)   ::Vanta::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client
#define VA_TRACE(...)        ::Vanta::Log::GetClientLogger()->trace(__VA_ARGS__)
#define VA_INFO(...)         ::Vanta::Log::GetClientLogger()->info(__VA_ARGS__)
#define VA_WARN(...)         ::Vanta::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VA_ERROR(...)        ::Vanta::Log::GetClientLogger()->error(__VA_ARGS__)
#define VA_CRITICAL(...)        ::Vanta::Log::GetClientLogger()->critical(__VA_ARGS__)