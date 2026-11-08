#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *LightMeta = "SIMP.Light";

        int LightGetColor(lua_State *L)
        {
            const LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            lua_pushinteger(L, light->color.r);
            lua_pushinteger(L, light->color.g);
            lua_pushinteger(L, light->color.b);
            return 3;
        }

        int LightSetColor(lua_State *L)
        {
            LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            const Uint8 r = ToByte(L, 2);
            const Uint8 g = ToByte(L, 3);
            const Uint8 b = ToByte(L, 4);
            light->color = SDL_Color{r, g, b, light->color.a};
            return 0;
        }

        int LightGetIntensity(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<LightComponent>(L, LightMeta)->intensity);
            return 1;
        }

        int LightSetIntensity(lua_State *L)
        {
            LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            light->intensity = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int LightGetRadius(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<LightComponent>(L, LightMeta)->radius);
            return 1;
        }

        int LightSetRadius(lua_State *L)
        {
            LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            light->radius = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int LightIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<LightComponent>(L, LightMeta)->enabled);
            return 1;
        }

        int LightSetEnabled(lua_State *L)
        {
            LightComponent *light = CheckComponent<LightComponent>(L, LightMeta);
            luaL_checkany(L, 2);
            light->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg LightMethods[] = {
            {"GetColor", LightGetColor},
            {"SetColor", LightSetColor},
            {"GetIntensity", LightGetIntensity},
            {"SetIntensity", LightSetIntensity},
            {"GetRadius", LightGetRadius},
            {"SetRadius", LightSetRadius},
            {"IsEnabled", LightIsEnabled},
            {"SetEnabled", LightSetEnabled},
            {nullptr, nullptr}};
    }

    const ComponentBinding &LightBinding()
    {
        static const ComponentBinding binding{"Light", LightMeta, LightMethods, &HasOf<LightComponent>, &AddOf<LightComponent>, &RemoveOf<LightComponent>};
        return binding;
    }
}
