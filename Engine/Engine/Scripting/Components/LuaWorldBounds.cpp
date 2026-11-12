#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *WorldBoundsMeta = "SIMP.WorldBounds";

        int WorldBoundsSetMin(lua_State *L)
        {
            WorldBoundsComponent *c = CheckComponent<WorldBoundsComponent>(L, WorldBoundsMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            c->min = {x, y};
            return 0;
        }
        int WorldBoundsSetMax(lua_State *L)
        {
            WorldBoundsComponent *c = CheckComponent<WorldBoundsComponent>(L, WorldBoundsMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            c->max = {x, y};
            return 0;
        }
        int WorldBoundsGetMin(lua_State *L)
        {
            const WorldBoundsComponent *c = CheckComponent<WorldBoundsComponent>(L, WorldBoundsMeta);
            lua_pushnumber(L, c->min.x);
            lua_pushnumber(L, c->min.y);
            return 2;
        }
        int WorldBoundsGetMax(lua_State *L)
        {
            const WorldBoundsComponent *c = CheckComponent<WorldBoundsComponent>(L, WorldBoundsMeta);
            lua_pushnumber(L, c->max.x);
            lua_pushnumber(L, c->max.y);
            return 2;
        }
        int WorldBoundsSetMargin(lua_State *L)
        {
            WorldBoundsComponent *c = CheckComponent<WorldBoundsComponent>(L, WorldBoundsMeta);
            c->margin = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int WorldBoundsSetEnabled(lua_State *L)
        {
            WorldBoundsComponent *c = CheckComponent<WorldBoundsComponent>(L, WorldBoundsMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int WorldBoundsIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<WorldBoundsComponent>(L, WorldBoundsMeta)->enabled);
            return 1;
        }
        int WorldBoundsSetMode(lua_State *L)
        {
            WorldBoundsComponent *c = CheckComponent<WorldBoundsComponent>(L, WorldBoundsMeta);
            const char *mode = luaL_checkstring(L, 2);
            const std::string_view name(mode);
            if (name == "clamp")
                c->mode = BoundsMode::Clamp;
            else if (name == "wrap")
                c->mode = BoundsMode::Wrap;
            else if (name == "bounce")
                c->mode = BoundsMode::Bounce;
            else if (name == "destroy")
                c->mode = BoundsMode::Destroy;
            else
                return luaL_error(L, "mode must be clamp, wrap, bounce or destroy");
            return 0;
        }

        const luaL_Reg WorldBoundsMethods[] = {
            {"SetMin", WorldBoundsSetMin},
            {"SetMax", WorldBoundsSetMax},
            {"GetMin", WorldBoundsGetMin},
            {"GetMax", WorldBoundsGetMax},
            {"SetMode", WorldBoundsSetMode},
            {"SetMargin", WorldBoundsSetMargin},
            {"SetEnabled", WorldBoundsSetEnabled},
            {"IsEnabled", WorldBoundsIsEnabled},
            {nullptr, nullptr}};
    }

    const ComponentBinding &WorldBoundsBinding()
    {
        static const ComponentBinding binding{"WorldBounds", WorldBoundsMeta, WorldBoundsMethods, &HasOf<WorldBoundsComponent>, &AddOf<WorldBoundsComponent>, &RemoveOf<WorldBoundsComponent>};
        return binding;
    }
}
