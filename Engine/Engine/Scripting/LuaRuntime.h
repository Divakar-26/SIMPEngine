#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

struct lua_State;

namespace SIMPEngine
{
    class LuaRuntime
    {
    public:
        static LuaRuntime &Get();

        LuaRuntime();
        ~LuaRuntime();

        LuaRuntime(const LuaRuntime &) = delete;
        LuaRuntime &operator=(const LuaRuntime &) = delete;

        bool Initialize();
        void ShutDown();

        bool IsInitialized() const { return m_State != nullptr; }
        uint64_t GetGeneration() const { return m_Generation; }
        lua_State *GetState() const { return m_State; }

        bool ExecuteFile(const std::string &path);
        bool ExecuteString(const std::string &source, const std::string &name = "string");

        bool Call(int argCount, int resultCount);

        const std::string &GetLastError() const { return m_LastError; }
        void ClearLastError() { m_LastError.clear(); }

        bool PushScriptClass(const std::string &path);
        void ClearScriptCache();

    private:
        struct ScriptClass
        {
            int Ref = 0;
            std::filesystem::file_time_type Time{};
        };

        bool LoadFile(const std::string &path);
        void RegisterBindings();
        void RegisterLibraries();

        lua_State *m_State = nullptr;
        uint64_t m_Generation = 0;
        std::string m_LastError;
        std::unordered_map<std::string, ScriptClass> m_ScriptClasses;
    };
}
