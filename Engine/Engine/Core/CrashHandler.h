#pragma once

#include <functional>
#include <string>

namespace SIMPEngine
{
    class CrashHandler
    {
    public:
        static void Install(const std::string &appName);
        static void SetEmergencyCallback(std::function<void()> callback);

        static const std::string &ReportDirectory();
        static std::string TakePreviousCrashReport();
        static void Fatal(const char *reason);
    };
}
