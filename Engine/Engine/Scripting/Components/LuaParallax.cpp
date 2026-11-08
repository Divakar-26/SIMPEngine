#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *ParallaxMeta = "SIMP.Parallax";

        int ParallaxGetFactor(lua_State *L)
        {
            const ParallaxComponent *parallax = CheckComponent<ParallaxComponent>(L, ParallaxMeta);
            lua_pushnumber(L, parallax->factor.x);
            lua_pushnumber(L, parallax->factor.y);
            return 2;
        }

        int ParallaxSetFactor(lua_State *L)
        {
            ParallaxComponent *parallax = CheckComponent<ParallaxComponent>(L, ParallaxMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_optnumber(L, 3, x));
            parallax->factor = {x, y};
            return 0;
        }

        const luaL_Reg ParallaxMethods[] = {
            {"GetFactor", ParallaxGetFactor},
            {"SetFactor", ParallaxSetFactor},
            {nullptr, nullptr}};
    }

    const ComponentBinding &ParallaxBinding()
    {
        static const ComponentBinding binding{"Parallax", ParallaxMeta, ParallaxMethods, &HasOf<ParallaxComponent>, &AddOf<ParallaxComponent>, &RemoveOf<ParallaxComponent>};
        return binding;
    }
}
