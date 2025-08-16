#pragma once
#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <vector>
#include <string>

class ImGuiSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    std::vector<std::string> Logs;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);
        Logs.push_back(fmt::to_string(formatted));
    }

    void flush_() override {}
};
