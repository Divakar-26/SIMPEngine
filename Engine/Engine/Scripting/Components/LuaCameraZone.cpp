#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *CameraZoneMeta = "SIMP.CameraZone";

        int CameraZoneSetZoom(lua_State *L)
        {
            CameraZoneComponent *c = CheckComponent<CameraZoneComponent>(L, CameraZoneMeta);
            c->zoom = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int CameraZoneSetSpeed(lua_State *L)
        {
            CameraZoneComponent *c = CheckComponent<CameraZoneComponent>(L, CameraZoneMeta);
            c->speed = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int CameraZoneSetEnabled(lua_State *L)
        {
            CameraZoneComponent *c = CheckComponent<CameraZoneComponent>(L, CameraZoneMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int CameraZoneIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<CameraZoneComponent>(L, CameraZoneMeta)->enabled);
            return 1;
        }
        int CameraZoneSetBoundsMin(lua_State *L)
        {
            CameraZoneComponent *c = CheckComponent<CameraZoneComponent>(L, CameraZoneMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            c->boundsMin = {x, y};
            return 0;
        }
        int CameraZoneSetBoundsMax(lua_State *L)
        {
            CameraZoneComponent *c = CheckComponent<CameraZoneComponent>(L, CameraZoneMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            c->boundsMax = {x, y};
            return 0;
        }
        int CameraZoneSetUseBounds(lua_State *L)
        {
            CameraZoneComponent *c = CheckComponent<CameraZoneComponent>(L, CameraZoneMeta);
            luaL_checkany(L, 2);
            c->setBounds = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg CameraZoneMethods[] = {
            {"SetZoom", CameraZoneSetZoom},
            {"SetSpeed", CameraZoneSetSpeed},
            {"SetEnabled", CameraZoneSetEnabled},
            {"IsEnabled", CameraZoneIsEnabled},
            {"SetBoundsMin", CameraZoneSetBoundsMin},
            {"SetBoundsMax", CameraZoneSetBoundsMax},
            {"SetUseBounds", CameraZoneSetUseBounds},
            {nullptr, nullptr}};
    }

    const ComponentBinding &CameraZoneBinding()
    {
        static const ComponentBinding binding{"CameraZone", CameraZoneMeta, CameraZoneMethods, &HasOf<CameraZoneComponent>, &AddOf<CameraZoneComponent>, &RemoveOf<CameraZoneComponent>};
        return binding;
    }
}
