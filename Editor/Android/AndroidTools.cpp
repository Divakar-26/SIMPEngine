#include "AndroidTools.h"

#include <Engine/Core/Log.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace AndroidTools
{
    namespace
    {
        std::string Trim(const std::string &s)
        {
            size_t a = s.find_first_not_of(" \t\r\n");
            if (a == std::string::npos)
                return {};
            size_t b = s.find_last_not_of(" \t\r\n");
            return s.substr(a, b - a + 1);
        }

        std::string ExeName(const std::string &name)
        {
#ifdef _WIN32
            return name + ".exe";
#else
            return name;
#endif
        }

        fs::path FindOnPath(const std::string &name)
        {
            const char *path = std::getenv("PATH");
            if (!path)
                return {};
#ifdef _WIN32
            const char separator = ';';
#else
            const char separator = ':';
#endif
            std::stringstream stream(path);
            std::string dir;
            std::error_code ec;
            while (std::getline(stream, dir, separator))
            {
                if (dir.empty())
                    continue;
                fs::path candidate = fs::path(dir) / ExeName(name);
                if (fs::is_regular_file(candidate, ec))
                    return candidate;
            }
            return {};
        }

    }

    std::string Device::Label() const
    {
        std::string name = Model.empty() ? Serial : Model;
        for (char &c : name)
            if (c == '_')
                c = ' ';
        if (Emulator)
            return name + " (emulator)";
        return name;
    }

    std::vector<Device> ParseDevices(const std::string &text)
    {
        std::vector<Device> devices;
        std::stringstream stream(text);
        std::string line;
        while (std::getline(stream, line))
        {
            line = Trim(line);
            if (line.empty() || line[0] == '*' || line.rfind("List of devices", 0) == 0)
                continue;
            std::stringstream words(line);
            Device device;
            if (!(words >> device.Serial >> device.State))
                continue;
            std::string token;
            while (words >> token)
            {
                size_t colon = token.find(':');
                if (colon == std::string::npos)
                    continue;
                std::string key = token.substr(0, colon);
                std::string value = token.substr(colon + 1);
                if (key == "model")
                    device.Model = value;
                else if (key == "product")
                    device.Product = value;
            }
            device.Emulator = device.Serial.rfind("emulator-", 0) == 0;
            devices.push_back(device);
        }
        return devices;
    }

    void ParseAvds(const std::string &text, std::vector<std::string> &out)
    {
        std::stringstream stream(text);
        std::string line;
        while (std::getline(stream, line))
        {
            line = Trim(line);
            if (line.empty() || line.rfind("INFO", 0) == 0 || line.find(' ') != std::string::npos)
                continue;
            out.push_back(line);
        }
    }

    std::string SanitizePackage(const std::string &projectName)
    {
        std::string part;
        for (char c : projectName)
        {
            if (std::isalnum(static_cast<unsigned char>(c)))
                part += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (part.empty() || std::isdigit(static_cast<unsigned char>(part[0])))
            part = "game" + part;
        return "org.simp." + part;
    }

    std::string JoinAbis(const std::vector<std::string> &abis)
    {
        std::vector<std::string> unique;
        for (const std::string &abi : abis)
            if (!abi.empty() && std::find(unique.begin(), unique.end(), abi) == unique.end())
                unique.push_back(abi);
        std::string result;
        for (size_t i = 0; i < unique.size(); i++)
            result += (i ? "," : "") + unique[i];
        return result;
    }

    namespace
    {
        std::string Env(const char *name)
        {
            const char *value = std::getenv(name);
            return value ? value : "";
        }

        bool IsDir(const fs::path &path)
        {
            std::error_code ec;
            return !path.empty() && fs::is_directory(path, ec);
        }

        bool IsFile(const fs::path &path)
        {
            std::error_code ec;
            return !path.empty() && fs::is_regular_file(path, ec);
        }

        fs::path Home()
        {
            std::string home = Env("HOME");
            if (home.empty())
                home = Env("USERPROFILE");
            return home;
        }

        fs::path ReadSdkDir(const fs::path &properties)
        {
            std::ifstream file(properties);
            std::string line;
            while (std::getline(file, line))
            {
                if (line.rfind("sdk.dir=", 0) == 0)
                {
                    std::string value = Trim(line.substr(8));
                    std::string clean;
                    for (size_t i = 0; i < value.size(); i++)
                    {
                        if (value[i] == '\\' && i + 1 < value.size() && (value[i + 1] == '\\' || value[i + 1] == ':'))
                            continue;
                        clean += value[i];
                    }
                    return clean;
                }
            }
            return {};
        }

        fs::path FindNdk(const fs::path &sdk, std::string &version)
        {
            for (const char *var : {"ANDROID_NDK_HOME", "ANDROID_NDK_ROOT"})
            {
                fs::path path = Env(var);
                if (IsDir(path))
                {
                    version = path.filename().string();
                    return path;
                }
            }
            if (sdk.empty())
                return {};
            std::vector<fs::path> found;
            std::error_code ec;
            if (IsDir(sdk / "ndk"))
                for (const auto &entry : fs::directory_iterator(sdk / "ndk", ec))
                    if (entry.is_directory(ec))
                        found.push_back(entry.path());
            std::sort(found.begin(), found.end());
            if (!found.empty())
            {
                version = found.back().filename().string();
                return found.back();
            }
            if (IsDir(sdk / "ndk-bundle"))
            {
                version = "ndk-bundle";
                return sdk / "ndk-bundle";
            }
            return {};
        }

        fs::path FindJavaHome()
        {
            fs::path javaHome = Env("JAVA_HOME");
            if (IsFile(javaHome / "bin" / ExeName("java")))
                return javaHome;
            fs::path java = FindOnPath("java");
            if (!java.empty())
            {
                std::error_code ec;
                fs::path resolved = fs::weakly_canonical(java, ec);
                if (IsFile(resolved))
                    return resolved.parent_path().parent_path();
            }
            std::vector<fs::path> studios = {
                "/opt/android-studio", "/usr/local/android-studio", "/snap/android-studio/current/android-studio",
                "C:/Program Files/Android/Android Studio", "/Applications/Android Studio.app/Contents"};
            if (!Home().empty())
            {
                studios.push_back(Home() / "android-studio");
                studios.push_back(Home() / "Applications" / "Android Studio.app" / "Contents");
            }
            for (const fs::path &studio : studios)
            {
                for (const fs::path &jbr : {studio / "jbr", studio / "jbr" / "Contents" / "Home", studio / "jre"})
                    if (IsFile(jbr / "bin" / ExeName("java")))
                        return jbr;
            }
            for (const fs::path &root : {fs::path("/usr/lib/jvm"), fs::path("C:/Program Files/Java"), fs::path("C:/Program Files/Eclipse Adoptium"), fs::path("/Library/Java/JavaVirtualMachines")})
            {
                std::error_code ec;
                if (!IsDir(root))
                    continue;
                std::vector<fs::path> jdks;
                for (const auto &entry : fs::directory_iterator(root, ec))
                {
                    fs::path home = entry.path();
                    if (IsFile(home / "Contents" / "Home" / "bin" / ExeName("java")))
                        home = home / "Contents" / "Home";
                    if (IsFile(home / "bin" / ExeName("java")) && IsFile(home / "bin" / ExeName("javac")))
                        jdks.push_back(home);
                }
                std::sort(jdks.begin(), jdks.end());
                if (!jdks.empty())
                    return jdks.back();
            }
            return {};
        }
    }

    bool IsSdkDirectory(const fs::path &path)
    {
        return IsDir(path) && (IsDir(path / "platform-tools") || IsDir(path / "platforms") || IsDir(path / "build-tools"));
    }

    std::vector<fs::path> SdkCandidates()
    {
        std::vector<fs::path> list;
        for (const char *var : {"ANDROID_HOME", "ANDROID_SDK_ROOT"})
            if (!Env(var).empty())
                list.push_back(Env(var));
        const fs::path home = Home();
        if (!Env("LOCALAPPDATA").empty())
            list.push_back(fs::path(Env("LOCALAPPDATA")) / "Android" / "Sdk");
        if (!home.empty())
        {
            list.push_back(home / "AppData" / "Local" / "Android" / "Sdk");
            list.push_back(home / "Android" / "Sdk");
            list.push_back(home / "Android" / "sdk");
            list.push_back(home / ".android" / "sdk");
            list.push_back(home / "Library" / "Android" / "sdk");
            list.push_back(home / "scoop" / "apps" / "android-sdk" / "current");
            list.push_back(home / "snap" / "android-studio" / "common" / "Android" / "Sdk");
        }
        for (const char *path : {"C:/Android/Sdk", "C:/Android/android-sdk", "C:/Android", "/usr/lib/android-sdk", "/usr/local/lib/android/sdk", "/opt/android-sdk", "/opt/android-sdk-linux", "/opt/android", "/usr/local/share/android-sdk", "/opt/homebrew/share/android-commandlinetools", "/opt/homebrew/share/android-sdk"})
            list.push_back(path);
        for (const char *var : {"ProgramFiles(x86)", "ProgramFiles"})
            if (!Env(var).empty())
            {
                list.push_back(fs::path(Env(var)) / "Android" / "android-sdk");
                list.push_back(fs::path(Env(var)) / "Android" / "Sdk");
            }
        return list;
    }

    fs::path FindAndroidProject()
    {
        std::error_code ec;
        std::vector<fs::path> starts;
        if (const char *base = SDL_GetBasePath())
            starts.push_back(base);
        starts.push_back(fs::current_path(ec));
        for (fs::path dir : starts)
        {
            for (int i = 0; i < 8 && !dir.empty(); i++)
            {
                fs::path candidate = dir / "Android";
                const bool wrapper = fs::is_regular_file(candidate / "gradlew", ec) || fs::is_regular_file(candidate / "gradlew.bat", ec);
                if (wrapper && fs::is_regular_file(candidate / "tools" / "pack_project.py", ec))
                    return candidate;
                fs::path parent = dir.parent_path();
                if (parent == dir)
                    break;
                dir = parent;
            }
        }
        return {};
    }

    Toolchain Detect(const std::string &sdkOverride)
    {
        Toolchain tools;
        tools.AndroidProject = FindAndroidProject();

        if (!sdkOverride.empty() && IsSdkDirectory(sdkOverride))
            tools.Sdk = sdkOverride;
        if (tools.Sdk.empty())
            for (const fs::path &candidate : SdkCandidates())
                if (IsSdkDirectory(candidate))
                {
                    tools.Sdk = candidate;
                    break;
                }
        if (tools.Sdk.empty() && !tools.AndroidProject.empty())
        {
            fs::path fromProperties = ReadSdkDir(tools.AndroidProject / "local.properties");
            if (IsSdkDirectory(fromProperties))
                tools.Sdk = fromProperties;
        }

        fs::path adb = FindOnPath("adb");
        if (tools.Sdk.empty() && !adb.empty())
        {
            std::error_code ec;
            fs::path root = fs::weakly_canonical(adb, ec).parent_path().parent_path();
            if (IsSdkDirectory(root))
                tools.Sdk = root;
        }

        if (!tools.Sdk.empty() && IsFile(tools.Sdk / "platform-tools" / ExeName("adb")))
            tools.Adb = tools.Sdk / "platform-tools" / ExeName("adb");
        else
            tools.Adb = adb;

        if (!tools.Sdk.empty() && IsFile(tools.Sdk / "emulator" / ExeName("emulator")))
            tools.Emulator = tools.Sdk / "emulator" / ExeName("emulator");
        else
            tools.Emulator = FindOnPath("emulator");

        tools.Ndk = FindNdk(tools.Sdk, tools.NdkVersion);
        tools.JavaHome = FindJavaHome();
        tools.Cmake = FindOnPath("cmake");
        return tools;
    }
}

AndroidSession::~AndroidSession()
{
    Shutdown();
}

void AndroidSession::Initialize(const std::string &sdkOverride, bool enabled)
{
    if (m_Initialized)
        return;
    m_Initialized = true;
    m_Enabled = enabled;
    m_Tools = AndroidTools::Detect(sdkOverride);
    if (!m_Enabled || !m_Tools.CanDeploy())
        return;
    CORE_INFO("Android: adb found at {}", m_Tools.Adb.string());
    m_Quit = false;
    m_Poller = std::thread([this] { PollLoop(); });
}

void AndroidSession::Rescan(const std::string &sdkOverride, bool enabled)
{
    if (m_State == State::Running)
        return;
    Shutdown();
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Devices.clear();
        m_AvdList.clear();
    }
    Initialize(sdkOverride, enabled);
}

void AndroidSession::Shutdown()
{
    m_Quit = true;
    Cancel();
    if (m_Worker.joinable())
        m_Worker.join();
    if (m_Poller.joinable())
        m_Poller.join();
    m_Initialized = false;
}

void AndroidSession::Update()
{
    if (m_State == State::Succeeded || m_State == State::Failed)
    {
        if (m_Worker.joinable())
            m_Worker.join();
    }
}

std::vector<AndroidTools::Device> AndroidSession::Devices()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Devices;
}

std::vector<std::string> AndroidSession::Avds()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_AvdList;
}

bool AndroidSession::IsSelected(const std::string &serial) const
{
    return std::find(m_Selected.begin(), m_Selected.end(), serial) != m_Selected.end();
}

void AndroidSession::SetSelected(const std::string &serial, bool selected)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = std::find(m_Selected.begin(), m_Selected.end(), serial);
    if (selected && it == m_Selected.end())
        m_Selected.push_back(serial);
    else if (!selected && it != m_Selected.end())
        m_Selected.erase(it);
}

std::vector<std::string> AndroidSession::SelectedSerials(const std::vector<AndroidTools::Device> &devices) const
{
    std::vector<std::string> result;
    for (const AndroidTools::Device &device : devices)
        if (device.Ready() && IsSelected(device.Serial))
            result.push_back(device.Serial);
    if (result.empty())
    {
        for (const AndroidTools::Device &device : devices)
            if (device.Ready())
            {
                result.push_back(device.Serial);
                break;
            }
    }
    return result;
}

std::string AndroidSession::Status()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Status;
}

std::vector<std::string> AndroidSession::TakeLog()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::vector<std::string> lines;
    lines.swap(m_LogLines);
    return lines;
}

void AndroidSession::Log(const std::string &line)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_LogLines.push_back(line);
    if (m_LogLines.size() > 2000)
        m_LogLines.erase(m_LogLines.begin(), m_LogLines.begin() + 1000);
}

void AndroidSession::SetStatus(const std::string &status)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Status = status;
}

int AndroidSession::Run(const std::vector<std::string> &args, std::string *captured, bool log, const fs::path &workingDirectory)
{
    std::vector<const char *> argv;
    for (const std::string &arg : args)
        argv.push_back(arg.c_str());
    argv.push_back(nullptr);

    SDL_Environment *env = SDL_CreateEnvironment(true);
    if (env && !m_Tools.Sdk.empty())
    {
        SDL_SetEnvironmentVariable(env, "ANDROID_HOME", m_Tools.Sdk.string().c_str(), true);
        SDL_SetEnvironmentVariable(env, "ANDROID_SDK_ROOT", m_Tools.Sdk.string().c_str(), true);
    }
    if (env && !m_Tools.JavaHome.empty() && !SDL_GetEnvironmentVariable(env, "JAVA_HOME"))
        SDL_SetEnvironmentVariable(env, "JAVA_HOME", m_Tools.JavaHome.string().c_str(), true);
    const std::string directory = workingDirectory.string();

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, argv.data());
    SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
    SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_NULL);
    SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
    if (env)
        SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ENVIRONMENT_POINTER, env);
    if (!directory.empty())
        SDL_SetStringProperty(props, SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING, directory.c_str());
    SDL_Process *process = SDL_CreateProcessWithProperties(props);
    SDL_DestroyProperties(props);
    if (env)
        SDL_DestroyEnvironment(env);
    if (!process)
    {
        Log(std::string("Could not start ") + args[0] + ": " + SDL_GetError());
        return -1;
    }

    m_Current = process;
    SDL_IOStream *out = SDL_GetProcessOutput(process);
    std::string pending;
    char buffer[2048];
    auto flushLine = [&](const std::string &raw)
    {
        std::string line = raw;
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (line.empty())
            return;
        if (captured)
            *captured += line + "\n";
        if (log)
        {
            Log(line);
            SetStatus(line);
        }
    };
    while (out)
    {
        size_t got = SDL_ReadIO(out, buffer, sizeof(buffer));
        if (got == 0)
        {
            if (SDL_GetIOStatus(out) != SDL_IO_STATUS_NOT_READY)
                break;
            int code = 0;
            if (SDL_WaitProcess(process, false, &code))
            {
                got = SDL_ReadIO(out, buffer, sizeof(buffer));
                if (got == 0)
                    break;
            }
            else
            {
                SDL_Delay(10);
                continue;
            }
        }
        pending.append(buffer, got);
        size_t newline;
        while ((newline = pending.find('\n')) != std::string::npos)
        {
            flushLine(pending.substr(0, newline));
            pending.erase(0, newline + 1);
        }
    }
    flushLine(pending);

    int exitCode = -1;
    SDL_WaitProcess(process, true, &exitCode);
    m_Current = nullptr;
    SDL_DestroyProcess(process);
    return exitCode;
}

void AndroidSession::PollLoop()
{
    std::vector<std::string> avdNames;
    if (!m_Tools.Emulator.empty())
    {
        std::string text;
        Run({m_Tools.Emulator.string(), "-list-avds"}, &text, false);
        AndroidTools::ParseAvds(text, avdNames);
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_AvdList = avdNames;
    }

    auto last = std::chrono::steady_clock::time_point{};
    while (!m_Quit)
    {
        auto now = std::chrono::steady_clock::now();
        if (m_RefreshRequested || now - last > std::chrono::milliseconds(2500))
        {
            m_RefreshRequested = false;
            last = now;
            std::string text;
            int code = Run({m_Tools.Adb.string(), "devices", "-l"}, &text, false);
            if (code == 0)
            {
                std::vector<AndroidTools::Device> devices = AndroidTools::ParseDevices(text);
                std::vector<AndroidTools::Device> previous;
                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    previous = m_Devices;
                }
                for (AndroidTools::Device &device : devices)
                {
                    for (const AndroidTools::Device &old : previous)
                        if (old.Serial == device.Serial && old.Ready() == device.Ready())
                            device.Abi = old.Abi;
                    if (device.Ready() && device.Abi.empty())
                    {
                        std::string abi;
                        Run({m_Tools.Adb.string(), "-s", device.Serial, "shell", "getprop", "ro.product.cpu.abi"}, &abi, false);
                        while (!abi.empty() && (abi.back() == '\n' || abi.back() == '\r' || abi.back() == ' '))
                            abi.pop_back();
                        device.Abi = abi;
                    }
                }
                std::lock_guard<std::mutex> lock(m_Mutex);
                m_Devices = devices;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void AndroidSession::StartEmulator(const std::string &avd)
{
    if (m_Tools.Emulator.empty() || avd.empty())
        return;
    const std::string executable = m_Tools.Emulator.string();
    std::thread([this, executable, avd]
    {
        Log("Starting emulator " + avd);
        Run({executable, "-avd", avd}, nullptr, false);
    }).detach();
}

bool AndroidSession::Deploy(const fs::path &projectDirectory, const std::string &projectName, const std::vector<std::string> &serials)
{
    if (m_State == State::Running || !m_Tools.CanDeploy() || serials.empty())
        return false;
    if (m_Worker.joinable())
        m_Worker.join();
    m_Cancel = false;
    m_State = State::Running;
    SetStatus("Starting");
    m_Worker = std::thread([this, projectDirectory, projectName, serials] { DeployJob(projectDirectory, projectName, serials); });
    return true;
}

void AndroidSession::Cancel()
{
    m_Cancel = true;
    if (void *current = m_Current.load())
        SDL_KillProcess(static_cast<SDL_Process *>(current), true);
}

void AndroidSession::DeployJob(fs::path projectDirectory, std::string projectName, std::vector<std::string> serials)
{
    const std::string appId = AndroidTools::SanitizePackage(projectName);
    std::vector<AndroidTools::Device> known = Devices();
    std::vector<std::string> abis;
    for (const std::string &serial : serials)
        for (const AndroidTools::Device &device : known)
            if (device.Serial == serial)
                abis.push_back(device.Abi);
    const std::string abiList = AndroidTools::JoinAbis(abis);

    auto fail = [this](const std::string &message)
    {
        Log(message);
        SetStatus(message);
        m_State = State::Failed;
    };

    if (m_Tools.AndroidProject.empty())
    {
        fail("Android project folder not found next to the editor");
        return;
    }
    const std::string abiArg = abiList.empty() ? std::string() : "-Pabis=" + abiList;
    std::string output;
    Log("Building " + projectName + " for " + (abiList.empty() ? "all ABIs" : abiList));
    SetStatus("Building");
    int code = 0;
#ifdef _WIN32
    {
        std::error_code ec;
        const fs::path properties = m_Tools.AndroidProject / "local.properties";
        std::string existing;
        {
            std::ifstream in(properties);
            std::stringstream buffer;
            buffer << in.rdbuf();
            existing = buffer.str();
        }
        if (!fs::is_directory(m_Tools.Sdk / "cmake", ec) && !m_Tools.Cmake.empty() && existing.find("cmake.dir=") == std::string::npos)
        {
            std::ofstream out(properties, std::ios::app);
            out << "cmake.dir=" << m_Tools.Cmake.parent_path().parent_path().generic_string() << "\n";
        }
    }
    code = Run({"python", (m_Tools.AndroidProject / "tools" / "pack_project.py").string(), projectDirectory.string()}, nullptr, true, m_Tools.AndroidProject);
    if (code != 0)
        code = Run({"py", "-3", (m_Tools.AndroidProject / "tools" / "pack_project.py").string(), projectDirectory.string()}, nullptr, true, m_Tools.AndroidProject);
    if (code == 0)
    {
        std::vector<std::string> gradle = {(m_Tools.AndroidProject / "gradlew.bat").string(), "assembleDebug", "-PappId=" + appId};
        if (!abiArg.empty())
            gradle.push_back(abiArg);
        code = Run(gradle, nullptr, true, m_Tools.AndroidProject);
        output = (m_Tools.AndroidProject / "app" / "build" / "outputs" / "apk" / "debug" / "app-debug.apk").string() + "\n";
    }
#else
    std::vector<std::string> build = {"bash", (m_Tools.AndroidProject / "build.sh").string(), projectDirectory.string(), "debug", "-PappId=" + appId};
    if (!abiArg.empty())
        build.push_back(abiArg);
    code = Run(build, &output, true);
#endif
    if (m_Cancel)
    {
        fail("Cancelled");
        return;
    }
    if (code != 0)
    {
        fail("Build failed");
        return;
    }

    fs::path apk;
    std::stringstream lines(output);
    std::string line;
    while (std::getline(lines, line))
        if (line.size() > 4 && line.compare(line.size() - 4, 4, ".apk") == 0)
            apk = line;
    if (apk.is_relative())
        apk = m_Tools.AndroidProject / apk;
    std::error_code ec;
    if (apk.empty() || !fs::is_regular_file(apk, ec))
    {
        fail("The build finished but no APK was found");
        return;
    }

    bool allOk = true;
    for (const std::string &serial : serials)
    {
        if (m_Cancel)
        {
            fail("Cancelled");
            return;
        }
        std::string label = serial;
        for (const AndroidTools::Device &device : known)
            if (device.Serial == serial)
                label = device.Label();
        Log("Installing on " + label);
        SetStatus("Installing on " + label);
        if (Run({m_Tools.Adb.string(), "-s", serial, "install", "-r", apk.string()}, nullptr, true) != 0)
        {
            Log("Install failed on " + label);
            allOk = false;
            continue;
        }
        Run({m_Tools.Adb.string(), "-s", serial, "shell", "am", "force-stop", appId}, nullptr, false);
        SetStatus("Launching on " + label);
        if (Run({m_Tools.Adb.string(), "-s", serial, "shell", "am", "start", "-n", appId + "/org.simp.player.PlayerActivity"}, nullptr, true) != 0)
        {
            Log("Launch failed on " + label);
            allOk = false;
        }
    }
    if (allOk)
    {
        SetStatus("Running on " + std::to_string(serials.size()) + (serials.size() == 1 ? " device" : " devices"));
        Log(Status());
        m_State = State::Succeeded;
    }
    else
        fail("Install or launch failed on a device");
}
