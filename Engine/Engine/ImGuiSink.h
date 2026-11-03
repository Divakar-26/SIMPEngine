#pragma once

#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <vector>
#include <string>

namespace SIMPEngine
{
    class ImGuiSink : public spdlog::sinks::base_sink<std::mutex>
    {
    private:
        static constexpr size_t MAX_LOGS = 10000;

    public:
        std::vector<std::string> Logs;

        void ClearOldLogs()
        {
            if (Logs.size() > MAX_LOGS)
            {
                Logs.erase(Logs.begin(), Logs.begin() + (Logs.size() - MAX_LOGS / 2));
            }
        }

    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override
        {
            spdlog::memory_buf_t formatted;
            base_sink<std::mutex>::formatter_->format(msg, formatted);
            Logs.push_back(fmt::to_string(formatted));
            ClearOldLogs();
        }

        void flush_() override {}
    };
}
