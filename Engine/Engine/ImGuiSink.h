#pragma once

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/os.h>
#include <chrono>
#include <ctime>
#include <mutex>
#include <vector>
#include <string>
#include <functional>

namespace SIMPEngine
{
    struct LogEntry
    {
        spdlog::level::level_enum Level = spdlog::level::info;
        std::string Time;
        std::string Logger;
        std::string Message;
    };

    class ImGuiSink : public spdlog::sinks::base_sink<std::mutex>
    {
    private:
        static constexpr size_t MAX_LOGS = 10000;

    public:
        void ForEach(const std::function<void(const std::vector<LogEntry> &)> &fn)
        {
            std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
            fn(m_Entries);
        }

        void Clear()
        {
            std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
            m_Entries.clear();
        }

    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override
        {
            LogEntry entry;
            entry.Level = msg.level;
            entry.Logger.assign(msg.logger_name.data(), msg.logger_name.size());
            entry.Message.assign(msg.payload.data(), msg.payload.size());

            std::time_t t = std::chrono::system_clock::to_time_t(msg.time);
            std::tm tm = spdlog::details::os::localtime(t);
            char buf[16];
            std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
            entry.Time = buf;

            m_Entries.push_back(std::move(entry));
            if (m_Entries.size() > MAX_LOGS)
                m_Entries.erase(m_Entries.begin(), m_Entries.begin() + (m_Entries.size() - MAX_LOGS / 2));
        }

        void flush_() override {}

    private:
        std::vector<LogEntry> m_Entries;
    };
}
