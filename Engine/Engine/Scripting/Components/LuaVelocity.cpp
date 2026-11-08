#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *VelocityMeta = "SIMP.Velocity";

        int VelocityGet(lua_State *L)
        {
            const VelocityComponent *velocity = CheckComponent<VelocityComponent>(L, VelocityMeta);
            lua_pushnumber(L, velocity->vx);
            lua_pushnumber(L, velocity->vy);
            return 2;
        }

        int VelocitySet(lua_State *L)
        {
            VelocityComponent *velocity = CheckComponent<VelocityComponent>(L, VelocityMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            velocity->vx = x;
            velocity->vy = y;
            return 0;
        }

        int VelocityAdd(lua_State *L)
        {
            VelocityComponent *velocity = CheckComponent<VelocityComponent>(L, VelocityMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            velocity->vx += x;
            velocity->vy += y;
            return 0;
        }

        const luaL_Reg VelocityMethods[] = {
            {"Get", VelocityGet},
            {"Set", VelocitySet},
            {"Add", VelocityAdd},
            {nullptr, nullptr}};
    }

    const ComponentBinding &VelocityBinding()
    {
        static const ComponentBinding binding{"Velocity", VelocityMeta, VelocityMethods, &HasOf<VelocityComponent>, &AddOf<VelocityComponent>, &RemoveOf<VelocityComponent>};
        return binding;
    }
}
