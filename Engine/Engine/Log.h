#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include "ImGuiSink.h"

namespace SIMPEngine
{

    class Log
    {
    public:
        static void Init();

        inline static std::shared_ptr<spdlog::logger> &GetCoreLogger() { return s_CoreLogger; }
        inline static std::shared_ptr<spdlog::logger> &GetClientLogger() { return s_ClientLogger; }
        inline static std::shared_ptr<ImGuiSink> &GetImGuiSink() { return s_ImGuiSink; }

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
        static std::shared_ptr<spdlog::logger> s_ClientLogger;
        static std::shared_ptr<ImGuiSink> s_ImGuiSink;
    };

}

#define CORE_TRACE(...) ::SIMPEngine::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define CORE_INFO(...) ::SIMPEngine::Log::GetCoreLogger()->info(__VA_ARGS__)
#define CORE_WARN(...) ::SIMPEngine::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define CORE_ERROR(...) ::SIMPEngine::Log::GetCoreLogger()->error(__VA_ARGS__)
#define CORE_CRITICAL(...) ::SIMPEngine::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define LOG_TRACE(...) ::SIMPEngine::Log::GetClientLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...) ::SIMPEngine::Log::GetClientLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::SIMPEngine::Log::GetClientLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::SIMPEngine::Log::GetClientLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::SIMPEngine::Log::GetClientLogger()->critical(__VA_ARGS__)
