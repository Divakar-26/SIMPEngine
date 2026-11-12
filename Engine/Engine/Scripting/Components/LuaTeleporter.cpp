#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *TeleporterMeta = "SIMP.Teleporter";

        int TeleporterSetTarget(lua_State *L)
        {
            TeleporterComponent *c = CheckComponent<TeleporterComponent>(L, TeleporterMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->target.assign(text, length);
            return 0;
        }
        int TeleporterGetTarget(lua_State *L)
        {
            const TeleporterComponent *c = CheckComponent<TeleporterComponent>(L, TeleporterMeta);
            lua_pushlstring(L, c->target.data(), c->target.size());
            return 1;
        }
        int TeleporterSetOffset(lua_State *L)
        {
            TeleporterComponent *c = CheckComponent<TeleporterComponent>(L, TeleporterMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            c->offset = {x, y};
            return 0;
        }
        int TeleporterGetOffset(lua_State *L)
        {
            const TeleporterComponent *c = CheckComponent<TeleporterComponent>(L, TeleporterMeta);
            lua_pushnumber(L, c->offset.x);
            lua_pushnumber(L, c->offset.y);
            return 2;
        }
        int TeleporterSetEnabled(lua_State *L)
        {
            TeleporterComponent *c = CheckComponent<TeleporterComponent>(L, TeleporterMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int TeleporterIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<TeleporterComponent>(L, TeleporterMeta)->enabled);
            return 1;
        }
        int TeleporterSetKeepVelocity(lua_State *L)
        {
            TeleporterComponent *c = CheckComponent<TeleporterComponent>(L, TeleporterMeta);
            luaL_checkany(L, 2);
            c->keepVelocity = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg TeleporterMethods[] = {
            {"SetTarget", TeleporterSetTarget},
            {"GetTarget", TeleporterGetTarget},
            {"SetOffset", TeleporterSetOffset},
            {"GetOffset", TeleporterGetOffset},
            {"SetEnabled", TeleporterSetEnabled},
            {"IsEnabled", TeleporterIsEnabled},
            {"SetKeepVelocity", TeleporterSetKeepVelocity},
            {nullptr, nullptr}};
    }

    const ComponentBinding &TeleporterBinding()
    {
        static const ComponentBinding binding{"Teleporter", TeleporterMeta, TeleporterMethods, &HasOf<TeleporterComponent>, &AddOf<TeleporterComponent>, &RemoveOf<TeleporterComponent>};
        return binding;
    }
}
