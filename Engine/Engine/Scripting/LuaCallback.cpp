#include "LuaCallback.h"
#include "LuaBindings.h"
#include "LuaInclude.h"
#include "LuaRuntime.h"

namespace SIMPEngine
{
    std::shared_ptr<LuaCallback> LuaCallback::FromStack(lua_State *L, int index)
    {
        if (lua_type(L, index) != LUA_TFUNCTION)
            return nullptr;
        lua_pushvalue(L, index);
        const int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        return std::shared_ptr<LuaCallback>(new LuaCallback(ref, LuaRuntime::Get().GetGeneration()));
    }

    LuaCallback::~LuaCallback()
    {
        LuaRuntime &runtime = LuaRuntime::Get();
        if (runtime.IsInitialized() && runtime.GetGeneration() == m_Generation)
            luaL_unref(runtime.GetState(), LUA_REGISTRYINDEX, m_Ref);
    }

    bool LuaCallback::Push(lua_State *&L) const
    {
        LuaRuntime &runtime = LuaRuntime::Get();
        if (!runtime.IsInitialized() || runtime.GetGeneration() != m_Generation)
            return false;
        L = runtime.GetState();
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_Ref);
        return true;
    }

    void LuaCallback::Call(Scene *scene) const
    {
        lua_State *L = nullptr;
        if (!Push(L))
            return;
        LuaBindings::SceneScope scope(scene);
        LuaRuntime::Get().Call(0, 0);
    }

    void LuaCallback::Call(Scene *scene, float value) const
    {
        lua_State *L = nullptr;
        if (!Push(L))
            return;
        lua_pushnumber(L, value);
        LuaBindings::SceneScope scope(scene);
        LuaRuntime::Get().Call(1, 0);
    }
}
