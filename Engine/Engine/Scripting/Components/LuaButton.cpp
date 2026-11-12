#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *ButtonMeta = "SIMP.Button";

        int ButtonSetEnabled(lua_State *L)
        {
            ButtonComponent *c = CheckComponent<ButtonComponent>(L, ButtonMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int ButtonIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<ButtonComponent>(L, ButtonMeta)->enabled);
            return 1;
        }
        int ButtonIsHovered(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<ButtonComponent>(L, ButtonMeta)->hovering);
            return 1;
        }
        int ButtonIsPressed(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<ButtonComponent>(L, ButtonMeta)->pressed);
            return 1;
        }
        int ButtonSetClickSound(lua_State *L)
        {
            ButtonComponent *c = CheckComponent<ButtonComponent>(L, ButtonMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->clickSound.assign(text, length);
            return 0;
        }
        int ButtonSetHoverSound(lua_State *L)
        {
            ButtonComponent *c = CheckComponent<ButtonComponent>(L, ButtonMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->hoverSound.assign(text, length);
            return 0;
        }
        int ButtonSetHoverScale(lua_State *L)
        {
            ButtonComponent *c = CheckComponent<ButtonComponent>(L, ButtonMeta);
            c->hoverScale = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int ButtonSetPressScale(lua_State *L)
        {
            ButtonComponent *c = CheckComponent<ButtonComponent>(L, ButtonMeta);
            c->pressScale = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        const luaL_Reg ButtonMethods[] = {
            {"SetEnabled", ButtonSetEnabled},
            {"IsEnabled", ButtonIsEnabled},
            {"IsHovered", ButtonIsHovered},
            {"IsPressed", ButtonIsPressed},
            {"SetClickSound", ButtonSetClickSound},
            {"SetHoverSound", ButtonSetHoverSound},
            {"SetHoverScale", ButtonSetHoverScale},
            {"SetPressScale", ButtonSetPressScale},
            {nullptr, nullptr}};
    }

    const ComponentBinding &ButtonBinding()
    {
        static const ComponentBinding binding{"Button", ButtonMeta, ButtonMethods, &HasOf<ButtonComponent>, &AddOf<ButtonComponent>, &RemoveOf<ButtonComponent>};
        return binding;
    }
}
