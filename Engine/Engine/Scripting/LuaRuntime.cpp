#include "LuaRuntime.h"
#include "LuaBindings.h"
#include "LuaInclude.h"

#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>

#include <cstring>
#include <fstream>
#include <optional>
#include <sstream>
#include <system_error>

namespace SIMPEngine
{
    namespace
    {
        int MessageHandler(lua_State *L)
        {
            const char *message = lua_tostring(L, 1);
            if (!message)
            {
                if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
                    return 1;
                message = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
            }
            luaL_traceback(L, L, message, 1);
            return 1;
        }

        int Panic(lua_State *L)
        {
            const char *message = lua_tostring(L, -1);
            CORE_CRITICAL("Lua panic: {}", message ? message : "unknown error");
            return 0;
        }

        std::optional<std::string> ReadFile(const std::string &realPath)
        {
            std::ifstream file(realPath, std::ios::binary);
            if (!file)
                return std::nullopt;

            std::ostringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }

    LuaRuntime &LuaRuntime::Get()
    {
        static LuaRuntime s_Instance;
        return s_Instance;
    }

    LuaRuntime::LuaRuntime() = default;

    LuaRuntime::~LuaRuntime()
    {
        ShutDown();
    }

    bool LuaRuntime::Initialize()
    {
        if (m_State)
            return true;

        m_State = luaL_newstate();
        if (!m_State)
        {
            CORE_ERROR("Lua: failed to create state");
            return false;
        }

        lua_atpanic(m_State, Panic);
        ++m_Generation;

        luaL_openlibs(m_State);
        RegisterLibraries();
        RegisterBindings();

        CORE_INFO("{} runtime ready", LUA_VERSION);
        return true;
    }

    void LuaRuntime::ShutDown()
    {
        if (!m_State)
            return;

        m_ScriptClasses.clear();
        lua_close(m_State);
        m_State = nullptr;
    }

    bool LuaRuntime::LoadFile(const std::string &path)
    {
        auto resolved = VFS::Resolve(path);
        if (!resolved)
        {
            CORE_ERROR("Lua: cannot resolve '{}'", path);
            return false;
        }

        auto text = ReadFile(*resolved);
        if (!text)
        {
            CORE_ERROR("Lua: cannot read '{}'", *resolved);
            return false;
        }

        size_t offset = 0;
        if (text->size() >= 3 && std::memcmp(text->data(), "\xEF\xBB\xBF", 3) == 0)
            offset = 3;

        const std::string chunkName = "@" + path;
        if (luaL_loadbufferx(m_State, text->data() + offset, text->size() - offset, chunkName.c_str(), "t") != LUA_OK)
        {
            m_LastError = lua_tostring(m_State, -1);
            CORE_ERROR("Lua: {}", m_LastError);
            lua_pop(m_State, 1);
            return false;
        }

        return true;
    }

    bool LuaRuntime::Call(int argCount, int resultCount)
    {
        if (!m_State)
            return false;

        const int base = lua_gettop(m_State) - argCount;
        lua_pushcfunction(m_State, MessageHandler);
        lua_insert(m_State, base);

        const int status = lua_pcall(m_State, argCount, resultCount, base);
        lua_remove(m_State, base);

        if (status != LUA_OK)
        {
            const char *message = lua_tostring(m_State, -1);
            m_LastError = message ? message : "unknown Lua error";
            CORE_ERROR("Lua: {}", m_LastError);
            lua_pop(m_State, 1);
            return false;
        }

        return true;
    }

    bool LuaRuntime::ExecuteFile(const std::string &path)
    {
        if (!Initialize())
            return false;

        if (!LoadFile(path))
            return false;

        return Call(0, 0);
    }

    bool LuaRuntime::ExecuteString(const std::string &source, const std::string &name)
    {
        if (!Initialize())
            return false;

        const std::string chunkName = "=" + name;
        if (luaL_loadbufferx(m_State, source.data(), source.size(), chunkName.c_str(), "t") != LUA_OK)
        {
            CORE_ERROR("Lua: {}", lua_tostring(m_State, -1));
            lua_pop(m_State, 1);
            return false;
        }

        return Call(0, 0);
    }

    bool LuaRuntime::PushScriptClass(const std::string &path)
    {
        if (!Initialize())
            return false;

        std::error_code ec;
        std::filesystem::file_time_type time{};
        if (auto resolved = VFS::Resolve(path))
            time = std::filesystem::last_write_time(*resolved, ec);

        auto cached = m_ScriptClasses.find(path);
        if (cached != m_ScriptClasses.end() && !ec && cached->second.Time == time)
        {
            lua_rawgeti(m_State, LUA_REGISTRYINDEX, cached->second.Ref);
            return true;
        }

        if (!LoadFile(path))
            return false;

        if (!Call(0, 1))
            return false;

        if (!lua_istable(m_State, -1))
        {
            CORE_ERROR("Lua: '{}' must return a table", path);
            lua_pop(m_State, 1);
            return false;
        }

        lua_getfield(m_State, -1, "__index");
        const bool hasIndex = !lua_isnil(m_State, -1);
        lua_pop(m_State, 1);
        if (!hasIndex)
        {
            lua_pushvalue(m_State, -1);
            lua_setfield(m_State, -2, "__index");
        }

        lua_pushvalue(m_State, -1);
        const int ref = luaL_ref(m_State, LUA_REGISTRYINDEX);

        if (cached != m_ScriptClasses.end())
        {
            luaL_unref(m_State, LUA_REGISTRYINDEX, cached->second.Ref);
            cached->second = ScriptClass{ref, time};
        }
        else
        {
            m_ScriptClasses.emplace(path, ScriptClass{ref, time});
        }

        return true;
    }

    void LuaRuntime::ClearScriptCache()
    {
        if (!m_State)
            return;

        for (auto &[path, script] : m_ScriptClasses)
            luaL_unref(m_State, LUA_REGISTRYINDEX, script.Ref);
        m_ScriptClasses.clear();
    }

    void LuaRuntime::RegisterLibraries()
    {
        lua_getglobal(m_State, "package");
        lua_pushstring(m_State, "");
        lua_setfield(m_State, -2, "cpath");

        if (auto scripts = VFS::Resolve("assets://scripts"))
        {
            const std::string searchPath = *scripts + "/?.lua;" + *scripts + "/?/init.lua";
            lua_pushstring(m_State, searchPath.c_str());
            lua_setfield(m_State, -2, "path");
        }

        lua_pop(m_State, 1);
    }

    void LuaRuntime::RegisterBindings()
    {
        LuaBindings::Register(m_State);
    }
}
