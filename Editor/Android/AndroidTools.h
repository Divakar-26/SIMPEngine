#pragma once

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace AndroidTools
{
    struct Device
    {
        std::string Serial;
        std::string State;
        std::string Model;
        std::string Product;
        std::string Abi;
        bool Emulator = false;

        bool Ready() const { return State == "device"; }
        std::string Label() const;
    };

    std::vector<Device> ParseDevices(const std::string &text);
    void ParseAvds(const std::string &text, std::vector<std::string> &out);
    std::string SanitizePackage(const std::string &projectName);
    std::string JoinAbis(const std::vector<std::string> &abis);
    struct Toolchain
    {
        std::filesystem::path Sdk;
        std::filesystem::path Adb;
        std::filesystem::path Emulator;
        std::filesystem::path Ndk;
        std::string NdkVersion;
        std::filesystem::path JavaHome;
        std::filesystem::path Cmake;
        std::filesystem::path AndroidProject;

        bool CanDeploy() const { return !Adb.empty(); }
        bool CanBuild() const { return !Sdk.empty() && !JavaHome.empty() && !AndroidProject.empty(); }
    };

    bool IsSdkDirectory(const std::filesystem::path &path);
    std::vector<std::filesystem::path> SdkCandidates();
    std::filesystem::path FindAndroidProject();
    Toolchain Detect(const std::string &sdkOverride);
}

class AndroidSession
{
public:
    enum class State
    {
        Idle,
        Running,
        Succeeded,
        Failed
    };

    ~AndroidSession();

    void Initialize(const std::string &sdkOverride = {}, bool enabled = true);
    void Rescan(const std::string &sdkOverride, bool enabled);
    void Shutdown();
    void Update();

    bool Available() const { return m_Enabled && m_Tools.CanDeploy(); }
    bool CanBuild() const { return m_Tools.CanBuild(); }
    bool Enabled() const { return m_Enabled; }
    const AndroidTools::Toolchain &Tools() const { return m_Tools; }

    std::vector<AndroidTools::Device> Devices();
    std::vector<std::string> Avds();
    bool EmulatorAvailable() const { return !m_Tools.Emulator.empty(); }
    void StartEmulator(const std::string &avd);
    void RefreshNow() { m_RefreshRequested = true; }

    bool IsSelected(const std::string &serial) const;
    void SetSelected(const std::string &serial, bool selected);
    std::vector<std::string> SelectedSerials(const std::vector<AndroidTools::Device> &devices) const;

    bool Deploy(const std::filesystem::path &projectDirectory, const std::string &projectName, const std::vector<std::string> &serials);
    void Cancel();
    State GetState() const { return m_State; }
    std::string Status();
    std::vector<std::string> TakeLog();

private:
    void PollLoop();
    void DeployJob(std::filesystem::path projectDirectory, std::string projectName, std::vector<std::string> serials);
    int Run(const std::vector<std::string> &args, std::string *captured, bool log, const std::filesystem::path &workingDirectory = {});
    void Log(const std::string &line);
    void SetStatus(const std::string &status);

    AndroidTools::Toolchain m_Tools;
    bool m_Enabled = true;

    std::thread m_Poller;
    std::thread m_Worker;
    std::atomic<bool> m_Quit{false};
    std::atomic<bool> m_RefreshRequested{false};
    std::atomic<State> m_State{State::Idle};
    std::atomic<void *> m_Current{nullptr};
    std::atomic<bool> m_Cancel{false};

    std::mutex m_Mutex;
    std::vector<AndroidTools::Device> m_Devices;
    std::vector<std::string> m_AvdList;
    std::vector<std::string> m_Selected;
    std::vector<std::string> m_LogLines;
    std::string m_Status;
    bool m_Initialized = false;
};
