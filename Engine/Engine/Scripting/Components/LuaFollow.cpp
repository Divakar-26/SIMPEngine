#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *FollowMeta = "SIMP.Follow";

        int FollowSetTarget(lua_State *L)
        {
            FollowComponent *c = CheckComponent<FollowComponent>(L, FollowMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->target.assign(text, length);
            return 0;
        }
        int FollowGetTarget(lua_State *L)
        {
            const FollowComponent *c = CheckComponent<FollowComponent>(L, FollowMeta);
            lua_pushlstring(L, c->target.data(), c->target.size());
            return 1;
        }
        int FollowSetOffset(lua_State *L)
        {
            FollowComponent *c = CheckComponent<FollowComponent>(L, FollowMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            c->offset = {x, y};
            return 0;
        }
        int FollowGetOffset(lua_State *L)
        {
            const FollowComponent *c = CheckComponent<FollowComponent>(L, FollowMeta);
            lua_pushnumber(L, c->offset.x);
            lua_pushnumber(L, c->offset.y);
            return 2;
        }
        int FollowSetSmoothing(lua_State *L)
        {
            FollowComponent *c = CheckComponent<FollowComponent>(L, FollowMeta);
            c->smoothing = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int FollowSetStopDistance(lua_State *L)
        {
            FollowComponent *c = CheckComponent<FollowComponent>(L, FollowMeta);
            c->stopDistance = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int FollowSetEnabled(lua_State *L)
        {
            FollowComponent *c = CheckComponent<FollowComponent>(L, FollowMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int FollowIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<FollowComponent>(L, FollowMeta)->enabled);
            return 1;
        }

        const luaL_Reg FollowMethods[] = {
            {"SetTarget", FollowSetTarget},
            {"GetTarget", FollowGetTarget},
            {"SetOffset", FollowSetOffset},
            {"GetOffset", FollowGetOffset},
            {"SetSmoothing", FollowSetSmoothing},
            {"SetStopDistance", FollowSetStopDistance},
            {"SetEnabled", FollowSetEnabled},
            {"IsEnabled", FollowIsEnabled},
            {nullptr, nullptr}};
    }

    const ComponentBinding &FollowBinding()
    {
        static const ComponentBinding binding{"Follow", FollowMeta, FollowMethods, &HasOf<FollowComponent>, &AddOf<FollowComponent>, &RemoveOf<FollowComponent>};
        return binding;
    }
}
