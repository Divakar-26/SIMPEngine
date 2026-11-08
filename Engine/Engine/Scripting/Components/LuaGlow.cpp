#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *GlowMeta = "SIMP.Glow";

        int GlowGetColor(lua_State *L)
        {
            return PushRgba(L, CheckComponent<GlowComponent>(L, GlowMeta)->color);
        }

        int GlowSetColor(lua_State *L)
        {
            GlowComponent *glow = CheckComponent<GlowComponent>(L, GlowMeta);
            ReadRgba(L, 2, glow->color);
            return 0;
        }

        int GlowGetIntensity(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<GlowComponent>(L, GlowMeta)->intensity);
            return 1;
        }

        int GlowSetIntensity(lua_State *L)
        {
            GlowComponent *glow = CheckComponent<GlowComponent>(L, GlowMeta);
            glow->intensity = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int GlowGetSpread(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<GlowComponent>(L, GlowMeta)->spread);
            return 1;
        }

        int GlowSetSpread(lua_State *L)
        {
            GlowComponent *glow = CheckComponent<GlowComponent>(L, GlowMeta);
            glow->spread = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int GlowIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<GlowComponent>(L, GlowMeta)->enabled);
            return 1;
        }

        int GlowSetEnabled(lua_State *L)
        {
            GlowComponent *glow = CheckComponent<GlowComponent>(L, GlowMeta);
            luaL_checkany(L, 2);
            glow->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg GlowMethods[] = {
            {"GetColor", GlowGetColor},
            {"SetColor", GlowSetColor},
            {"GetIntensity", GlowGetIntensity},
            {"SetIntensity", GlowSetIntensity},
            {"GetSpread", GlowGetSpread},
            {"SetSpread", GlowSetSpread},
            {"IsEnabled", GlowIsEnabled},
            {"SetEnabled", GlowSetEnabled},
            {nullptr, nullptr}};
    }

    const ComponentBinding &GlowBinding()
    {
        static const ComponentBinding binding{"Glow", GlowMeta, GlowMethods, &HasOf<GlowComponent>, &AddOf<GlowComponent>, &RemoveOf<GlowComponent>};
        return binding;
    }
}
