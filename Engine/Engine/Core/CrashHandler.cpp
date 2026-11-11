#include <Engine/Core/CrashHandler.h>
#include <Engine/Core/FileSystem.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/Version.h>

#include <SDL3/SDL.h>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#if defined(__GLIBC__)
#include <execinfo.h>
#endif

#if defined(__SANITIZE_ADDRESS__)
#define SIMP_SANITIZED 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define SIMP_SANITIZED 1
#endif
#endif

namespace SIMPEngine
{
    namespace
    {
        std::string s_Directory;
        std::string s_ReportPath;
        std::string s_MarkerPath;
        std::function<void()> s_Emergency;
        std::atomic<int> s_Crashing{0};
        bool s_Installed = false;

        void WriteRaw(int fd, const char *text)
        {
            if (fd < 0 || !text)
                return;
#if defined(_WIN32)
            _write(fd, text, static_cast<unsigned int>(std::strlen(text)));
#else
            ssize_t ignored = write(fd, text, std::strlen(text));
            (void)ignored;
#endif
        }

        int OpenForWriting(const std::string &path)
        {
            if (path.empty())
                return -1;
#if defined(_WIN32)
            return _open(path.c_str(), _O_CREAT | _O_WRONLY | _O_TRUNC | _O_BINARY, 0644);
#else
            return open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
#endif
        }

        void CloseReport(int fd)
        {
            if (fd < 0)
                return;
#if defined(_WIN32)
            _close(fd);
#else
            close(fd);
#endif
        }

        const char *SignalName(int signal)
        {
            switch (signal)
            {
            case SIGSEGV:
                return "segmentation fault (invalid memory access)";
            case SIGABRT:
                return "abort";
            case SIGFPE:
                return "floating point exception";
            case SIGILL:
                return "illegal instruction";
#if !defined(_WIN32)
            case SIGBUS:
                return "bus error";
#endif
            default:
                return "fatal signal";
            }
        }

        void HandleCrash(const char *reason)
        {
            if (s_Crashing.fetch_add(1) != 0)
                return;

#if !defined(_WIN32)
            alarm(5);
#endif
            const int fd = OpenForWriting(s_ReportPath);
            WriteRaw(fd, EngineName);
            WriteRaw(fd, " ");
            WriteRaw(fd, EngineVersion);
            WriteRaw(fd, " crashed: ");
            WriteRaw(fd, reason);
            WriteRaw(fd, "\n\n");
#if defined(__GLIBC__)
            void *frames[64];
            const int count = backtrace(frames, 64);
            if (fd >= 0)
                backtrace_symbols_fd(frames, count, fd);
#endif
            CloseReport(fd);

            const int marker = OpenForWriting(s_MarkerPath);
            WriteRaw(marker, s_ReportPath.c_str());
            CloseReport(marker);

            const char stderrHeader[] = "\nSIMPEngine crashed. Report: ";
            WriteRaw(2, stderrHeader);
            WriteRaw(2, s_ReportPath.c_str());
            WriteRaw(2, "\n");

            if (s_Emergency)
                s_Emergency();
        }

#if !defined(_WIN32)
        void OnSignal(int signal)
        {
            HandleCrash(SignalName(signal));
            std::signal(signal, SIG_DFL);
            std::raise(signal);
        }

        [[maybe_unused]] void InstallSignals()
        {
            static char alternateStack[64 * 1024];
            stack_t stack{};
            stack.ss_sp = alternateStack;
            stack.ss_size = sizeof(alternateStack);
            sigaltstack(&stack, nullptr);

            struct sigaction action{};
            action.sa_handler = OnSignal;
            sigemptyset(&action.sa_mask);
            action.sa_flags = SA_ONSTACK | SA_RESETHAND;
            for (int signal : {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS})
                sigaction(signal, &action, nullptr);
        }
#else
        LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS *info)
        {
            const DWORD code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0;
            HandleCrash(code == EXCEPTION_ACCESS_VIOLATION ? "access violation (invalid memory access)"
                        : code == EXCEPTION_STACK_OVERFLOW ? "stack overflow"
                                                           : "unhandled exception");
            return EXCEPTION_CONTINUE_SEARCH;
        }

        void OnSignal(int signal)
        {
            HandleCrash(SignalName(signal));
            std::signal(signal, SIG_DFL);
            std::raise(signal);
        }

        [[maybe_unused]] void InstallSignals()
        {
            SetUnhandledExceptionFilter(OnUnhandledException);
            std::signal(SIGABRT, OnSignal);
        }
#endif

        void OnTerminate()
        {
            std::string reason = "unhandled C++ exception";
            if (std::exception_ptr current = std::current_exception())
            {
                try
                {
                    std::rethrow_exception(current);
                }
                catch (const std::exception &e)
                {
                    reason += ": ";
                    reason += e.what();
                }
                catch (...)
                {
                }
            }
            HandleCrash(reason.c_str());
            std::signal(SIGABRT, SIG_DFL);
            std::abort();
        }
    }

    void CrashHandler::Install(const std::string &appName)
    {
        if (s_Installed)
            return;
        s_Installed = true;

        if (char *pref = SDL_GetPrefPath("SIMPEngine", appName.c_str()))
        {
            s_Directory = FileSystem::Join(pref, "crashes");
            SDL_free(pref);
        }
        else
        {
            std::error_code ec;
            s_Directory = (std::filesystem::temp_directory_path(ec) / "SIMPEngine-crashes").string();
        }
        FileSystem::CreateDirectories(s_Directory);

        char stamp[32];
        const std::time_t now = std::time(nullptr);
        std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", std::localtime(&now));
        s_ReportPath = FileSystem::Join(s_Directory, std::string("crash-") + stamp + ".txt");
        s_MarkerPath = FileSystem::Join(s_Directory, "last-crash.txt");

        std::set_terminate(OnTerminate);
#if !defined(SIMP_SANITIZED)
        InstallSignals();
#endif
    }

    void CrashHandler::SetEmergencyCallback(std::function<void()> callback)
    {
        s_Emergency = std::move(callback);
    }

    const std::string &CrashHandler::ReportDirectory()
    {
        return s_Directory;
    }

    std::string CrashHandler::TakePreviousCrashReport()
    {
        if (s_MarkerPath.empty())
            return {};
        const std::optional<std::string> previous = FileSystem::ReadText(s_MarkerPath);
        FileSystem::Remove(s_MarkerPath);
        if (!previous || *previous == s_ReportPath || !FileSystem::Exists(*previous))
            return {};
        return *previous;
    }

    void CrashHandler::Fatal(const char *reason)
    {
        CORE_ERROR("Fatal: {}", reason);
        HandleCrash(reason);
        std::signal(SIGABRT, SIG_DFL);
        std::abort();
    }
}
