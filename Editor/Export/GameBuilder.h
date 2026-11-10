#pragma once

#include <Engine/Project/Project.h>

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class GameBuilder
{
public:
    enum class State
    {
        Idle,
        Running,
        Succeeded,
        Failed
    };

    struct Options
    {
        std::filesystem::path ProjectDirectory;
        std::filesystem::path OutputDirectory;
        SIMPEngine::ProjectSettings Settings;
        std::filesystem::path EngineResources;
    };

    ~GameBuilder();

    bool Start(const Options &options);
    State GetState() const { return m_State.load(); }
    float GetProgress() const { return m_Progress.load(); }
    std::vector<std::string> GetLog() const;
    std::filesystem::path GetExecutable() const;
    std::filesystem::path GetOutputDirectory() const { return m_Options.OutputDirectory; }

    static std::filesystem::path PlayerBinary();
    static std::string PlatformName();
    static std::string ExecutableName(const std::string &projectName);

private:
    void Run();
    void Log(const std::string &line);
    void Fail(const std::string &line);

    Options m_Options;
    std::thread m_Thread;
    std::atomic<State> m_State{State::Idle};
    std::atomic<float> m_Progress{0.0f};
    mutable std::mutex m_Mutex;
    std::vector<std::string> m_Log;
    std::filesystem::path m_Executable;
};
