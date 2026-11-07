#include "LuaScript.h"
#include "LuaBindings.h"
#include "LuaInclude.h"
#include "LuaRuntime.h"

#include <Engine/Core/Log.h>
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    static_assert(LuaScript::NoRef == LUA_NOREF);
}

namespace
{
    int CallHook(lua_State *L)
    {
        const char *name = lua_tostring(L, 2);
        const bool hasDelta = lua_gettop(L) >= 3;
        lua_getfield(L, 1, name);
        if (!lua_isfunction(L, -1))
            return 0;

        lua_pushvalue(L, 1);
        int argCount = 1;
        if (hasDelta)
        {
            lua_pushvalue(L, 3);
            argCount = 2;
        }
        lua_call(L, argCount, 0);
        return 0;
    }

    int ProbeHook(lua_State *L)
    {
        const char *name = lua_tostring(L, 2);
        lua_getfield(L, 1, name);
        lua_pushboolean(L, lua_isfunction(L, -1));
        return 1;
    }
}

namespace SIMPEngine
{
    LuaScript::LuaScript(std::string path) : m_Path(std::move(path))
    {
    }

    LuaScript::~LuaScript()
    {
        if (!IsLive())
            return;

        luaL_unref(LuaRuntime::Get().GetState(), LUA_REGISTRYINDEX, m_SelfRef);
        m_SelfRef = NoRef;
    }

    bool LuaScript::IsLive() const
    {
        const LuaRuntime &runtime = LuaRuntime::Get();
        return m_SelfRef != NoRef && runtime.IsInitialized() && runtime.GetGeneration() == m_Generation;
    }

    bool LuaScript::HasHook(const char *name) const
    {
        LuaRuntime &runtime = LuaRuntime::Get();
        lua_State *L = runtime.GetState();
        const int top = lua_gettop(L);

        lua_pushcfunction(L, ProbeHook);
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_SelfRef);
        lua_pushstring(L, name);

        bool found = false;
        if (runtime.Call(2, 1))
            found = lua_toboolean(L, -1) != 0;

        lua_settop(L, top);
        return found;
    }

    bool LuaScript::Invoke(const char *hook, bool passDelta, float dt)
    {
        LuaRuntime &runtime = LuaRuntime::Get();
        lua_State *L = runtime.GetState();
        const int top = lua_gettop(L);

        lua_pushcfunction(L, CallHook);
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_SelfRef);
        lua_pushstring(L, hook);
        int argCount = 2;
        if (passDelta)
        {
            lua_pushnumber(L, dt);
            argCount = 3;
        }

        const bool ok = runtime.Call(argCount, 0);
        lua_settop(L, top);
        return ok;
    }

    void LuaScript::Disable()
    {
        m_Failed = true;
        CORE_ERROR("Lua: script '{}' disabled after an error", m_Path);
    }

    void LuaScript::OnCreate()
    {
        LuaRuntime &runtime = LuaRuntime::Get();
        if (!runtime.PushScriptClass(m_Path))
        {
            Disable();
            return;
        }

        lua_State *L = runtime.GetState();

        lua_newtable(L);
        lua_pushvalue(L, -2);
        lua_setmetatable(L, -2);
        LuaBindings::PushEntity(L, GetEntity());
        lua_setfield(L, -2, "entity");
        lua_pushvalue(L, -1);
        m_SelfRef = luaL_ref(L, LUA_REGISTRYINDEX);
        m_Generation = runtime.GetGeneration();
        lua_pop(L, 2);

        if (!Invoke("OnCreate", false, 0.0f))
        {
            Disable();
            return;
        }

        m_HasUpdate = HasHook("OnUpdate");
        m_HasRender = HasHook("OnRender");
    }

    void LuaScript::OnDestroy()
    {
        if (m_Failed || !IsLive())
            return;

        Invoke("OnDestroy", false, 0.0f);
    }

    void LuaScript::OnUpdate(float dt)
    {
        if (m_Failed || !m_HasUpdate || !IsLive())
            return;

        if (!Invoke("OnUpdate", true, dt))
            Disable();
    }

    void LuaScript::OnRender()
    {
        if (m_Failed || !m_HasRender || !IsLive())
            return;

        if (!Invoke("OnRender", false, 0.0f))
            Disable();
    }
}

void ScriptComponent::BindLua(const std::string &path)
{
    InstantiateScript = [path]() -> std::unique_ptr<SIMPEngine::ScriptableEntity>
    {
        return std::make_unique<SIMPEngine::LuaScript>(path);
    };
}
