#include <Engine/Core/Log.h>

#include <spdlog/sinks/stdout_color_sinks.h>

namespace SIMPEngine
{

    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;
    std::shared_ptr<ImGuiSink> Log::s_ImGuiSink;

    void Log::Init()
    {
        spdlog::set_pattern("%^[%T] %n: %v%$");

        s_CoreLogger = spdlog::stdout_color_mt("ENGINE");
        s_CoreLogger->set_level(spdlog::level::trace);

        s_ClientLogger = spdlog::stdout_color_mt("APP");
        s_ClientLogger->set_level(spdlog::level::trace);

        s_ImGuiSink = std::make_shared<ImGuiSink>();
        s_CoreLogger->sinks().push_back(s_ImGuiSink);
        s_ClientLogger->sinks().push_back(s_ImGuiSink);
    }

}
