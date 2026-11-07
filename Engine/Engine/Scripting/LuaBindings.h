#pragma once

struct lua_State;

namespace SIMPEngine
{
    class Entity;

    namespace LuaBindings
    {
        void Register(lua_State *L);
        void PushEntity(lua_State *L, const Entity &entity);
    }
}
