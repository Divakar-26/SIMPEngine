#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *CameraMeta = "SIMP.Camera";

        int CameraShake(lua_State *L)
        {
            CameraComponent *camera = CheckComponent<CameraComponent>(L, CameraMeta);
            camera->Shake(static_cast<float>(luaL_checknumber(L, 2)));
            return 0;
        }

        int CameraGetZoom(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<CameraComponent>(L, CameraMeta)->Camera.GetZoom());
            return 1;
        }

        int CameraSetZoom(lua_State *L)
        {
            CameraComponent *camera = CheckComponent<CameraComponent>(L, CameraMeta);
            const float zoom = static_cast<float>(luaL_checknumber(L, 2));
            camera->Camera.SetZoom(zoom);
            return 0;
        }

        int CameraSetFollow(lua_State *L)
        {
            CameraComponent *camera = CheckComponent<CameraComponent>(L, CameraMeta);
            size_t length = 0;
            const char *name = luaL_checklstring(L, 2, &length);
            if (!lua_isnoneornil(L, 3))
            {
                const float x = static_cast<float>(luaL_checknumber(L, 3));
                const float y = static_cast<float>(luaL_optnumber(L, 4, 0.0));
                camera->followOffset = {x, y};
            }
            camera->followTarget.assign(name, length);
            camera->followHandle = entt::null;
            return 0;
        }

        int CameraIsPrimary(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<CameraComponent>(L, CameraMeta)->primary);
            return 1;
        }

        const luaL_Reg CameraMethods[] = {
            {"Shake", CameraShake},
            {"GetZoom", CameraGetZoom},
            {"SetZoom", CameraSetZoom},
            {"SetFollow", CameraSetFollow},
            {"IsPrimary", CameraIsPrimary},
            {nullptr, nullptr}};
    }

    const ComponentBinding &CameraBinding()
    {
        static const ComponentBinding binding{"Camera", CameraMeta, CameraMethods, &HasOf<CameraComponent>, &AddOf<CameraComponent>, &RemoveOf<CameraComponent>};
        return binding;
    }
}
