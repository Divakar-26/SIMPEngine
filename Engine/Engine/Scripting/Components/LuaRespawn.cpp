#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *RespawnMeta = "SIMP.Respawn";

        int RespawnGetPoint(lua_State *L)
        {
            const RespawnComponent *c = CheckComponent<RespawnComponent>(L, RespawnMeta);
            lua_pushnumber(L, c->point.x);
            lua_pushnumber(L, c->point.y);
            return 2;
        }

        int RespawnSetPoint(lua_State *L)
        {
            RespawnComponent *c = CheckComponent<RespawnComponent>(L, RespawnMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            c->point = {x, y};
            c->pointSet = true;
            return 0;
        }

        int RespawnNow(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<RespawnComponent>(L, RespawnMeta);
            RespawnSystem::Respawn(*entity->GetScene(), entity->GetHandle());
            return 0;
        }

        int RespawnSetDelay(lua_State *L)
        {
            RespawnComponent *c = CheckComponent<RespawnComponent>(L, RespawnMeta);
            c->delay = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int RespawnSetKillY(lua_State *L)
        {
            RespawnComponent *c = CheckComponent<RespawnComponent>(L, RespawnMeta);
            c->killY = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int RespawnSetUseKillY(lua_State *L)
        {
            RespawnComponent *c = CheckComponent<RespawnComponent>(L, RespawnMeta);
            luaL_checkany(L, 2);
            c->useKillY = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg RespawnMethods[] = {
            {"GetPoint", RespawnGetPoint},
            {"SetPoint", RespawnSetPoint},
            {"Respawn", RespawnNow},
            {"SetDelay", RespawnSetDelay},
            {"SetKillY", RespawnSetKillY},
            {"SetUseKillY", RespawnSetUseKillY},
            {nullptr, nullptr}};
    }

    const ComponentBinding &RespawnBinding()
    {
        static const ComponentBinding binding{"Respawn", RespawnMeta, RespawnMethods, &HasOf<RespawnComponent>, &AddOf<RespawnComponent>, &RemoveOf<RespawnComponent>};
        return binding;
    }
}
