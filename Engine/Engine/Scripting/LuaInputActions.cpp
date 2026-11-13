#include "LuaBindings.h"
#include "LuaHelpers.h"
#include "LuaInclude.h"
#include "LuaModules.h"

#include <Engine/Input/Input.h>
#include <Engine/Input/InputActions.h>
#include <Engine/Input/TouchControls.h>
#include <Engine/Scene/Scene.h>

#include <SDL3/SDL.h>

#include <string>
#include <vector>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        int InputIsActionDown(lua_State *L)
        {
            lua_pushboolean(L, InputActions::IsDown(luaL_checkstring(L, 1)));
            return 1;
        }

        int InputIsActionJustPressed(lua_State *L)
        {
            lua_pushboolean(L, InputActions::IsJustPressed(luaL_checkstring(L, 1)));
            return 1;
        }

        int InputIsActionJustReleased(lua_State *L)
        {
            lua_pushboolean(L, InputActions::IsJustReleased(luaL_checkstring(L, 1)));
            return 1;
        }

        int InputGetVector(lua_State *L)
        {
            float x = 0.0f, y = 0.0f;
            InputActions::GetVector(luaL_checkstring(L, 1), x, y);
            lua_pushnumber(L, x);
            lua_pushnumber(L, y);
            return 2;
        }

        int InputHasAction(lua_State *L)
        {
            lua_pushboolean(L, InputActions::Has(luaL_checkstring(L, 1)));
            return 1;
        }

        int InputSetActionValue(lua_State *L)
        {
            const char *name = luaL_checkstring(L, 1);
            const float value = static_cast<float>(luaL_checknumber(L, 2));
            InputActions::Inject(name, value);
            return 0;
        }

        int InputRebind(lua_State *L)
        {
            const char *name = luaL_checkstring(L, 1);
            const char *binding = luaL_checkstring(L, 2);
            lua_pushboolean(L, InputActions::Rebind(name, binding));
            return 1;
        }

        int InputAddBinding(lua_State *L)
        {
            const char *name = luaL_checkstring(L, 1);
            const char *binding = luaL_checkstring(L, 2);
            lua_pushboolean(L, InputActions::AddBinding(name, binding));
            return 1;
        }

        int InputRemoveBinding(lua_State *L)
        {
            const char *name = luaL_checkstring(L, 1);
            const char *binding = luaL_checkstring(L, 2);
            lua_pushboolean(L, InputActions::RemoveBinding(name, binding));
            return 1;
        }

        int InputClearBindings(lua_State *L)
        {
            lua_pushboolean(L, InputActions::ClearBindings(luaL_checkstring(L, 1)));
            return 1;
        }

        int InputResetBindings(lua_State *L)
        {
            if (lua_isnoneornil(L, 1))
            {
                InputActions::ResetAll();
                lua_pushboolean(L, 1);
            }
            else
                lua_pushboolean(L, InputActions::ResetAction(luaL_checkstring(L, 1)));
            return 1;
        }

        int InputGetBindings(lua_State *L)
        {
            const std::vector<std::string> bindings = InputActions::GetBindingTexts(luaL_checkstring(L, 1));
            lua_createtable(L, static_cast<int>(bindings.size()), 0);
            int index = 1;
            for (const std::string &text : bindings)
            {
                lua_pushstring(L, text.c_str());
                lua_rawseti(L, -2, index++);
            }
            return 1;
        }

        int InputDescribeBinding(lua_State *L)
        {
            const char *text = luaL_checkstring(L, 1);
            auto binding = InputActions::FromText(text);
            lua_pushstring(L, binding ? InputActions::DisplayText(*binding).c_str() : text);
            return 1;
        }

        int InputGetActions(lua_State *L)
        {
            const auto &actions = InputActions::GetActions();
            lua_createtable(L, static_cast<int>(actions.size()), 0);
            int index = 1;
            for (const InputAction &action : actions)
            {
                lua_pushstring(L, action.Name.c_str());
                lua_rawseti(L, -2, index++);
            }
            return 1;
        }

        int InputGetPressedBinding(lua_State *L)
        {
            auto text = InputActions::PollPressedBinding();
            if (!text)
            {
                lua_pushnil(L);
                return 1;
            }
            lua_pushstring(L, text->c_str());
            return 1;
        }

        int InputIsGamepadConnected(lua_State *L)
        {
            lua_pushboolean(L, Input::IsGamepadConnected());
            return 1;
        }

        int InputGetGamepadAxis(lua_State *L)
        {
            auto binding = InputActions::FromText(std::string("Axis.") + luaL_checkstring(L, 1));
            lua_pushnumber(L, binding ? Input::GetGamepadAxis(binding->Code) : 0.0f);
            return 1;
        }

        int InputIsGamepadDown(lua_State *L)
        {
            auto binding = InputActions::FromText(std::string("Pad.") + luaL_checkstring(L, 1));
            lua_pushboolean(L, binding && Input::IsGamepadButtonPressed(binding->Code));
            return 1;
        }

        int InputSetGamepadDeadzone(lua_State *L)
        {
            Input::SetGamepadDeadzone(static_cast<float>(luaL_checknumber(L, 1)));
            return 0;
        }

        int InputSetTouchControls(lua_State *L)
        {
            luaL_checkany(L, 1);
            TouchControls::SetVisible(lua_toboolean(L, 1) != 0);
            return 0;
        }

        int InputSetFloatingStick(lua_State *L)
        {
            luaL_checkany(L, 1);
            TouchControls::SetFloatingStick(lua_toboolean(L, 1) != 0);
            return 0;
        }

        void PushViewportAndWorld(lua_State *L, const glm::vec2 &window)
        {
            const glm::vec2 viewport = Input::WindowToViewport(window);
            lua_pushnumber(L, viewport.x);
            lua_setfield(L, -2, "x");
            lua_pushnumber(L, viewport.y);
            lua_setfield(L, -2, "y");
            if (Scene *scene = LuaBindings::GetActiveScene())
            {
                const glm::vec2 world = scene->ScreenToWorld(viewport);
                lua_pushnumber(L, world.x);
                lua_setfield(L, -2, "worldX");
                lua_pushnumber(L, world.y);
                lua_setfield(L, -2, "worldY");
            }
        }

        int InputGetTouches(lua_State *L)
        {
            const std::vector<TouchPoint> touches = TouchControls::GetTouches();
            lua_createtable(L, static_cast<int>(touches.size()), 0);
            int index = 1;
            for (const TouchPoint &touch : touches)
            {
                lua_createtable(L, 0, 7);
                lua_pushinteger(L, static_cast<lua_Integer>(touch.Id));
                lua_setfield(L, -2, "id");
                PushViewportAndWorld(L, touch.Position);
                lua_pushboolean(L, touch.Pressed);
                lua_setfield(L, -2, "pressed");
                lua_pushboolean(L, touch.Released);
                lua_setfield(L, -2, "released");
                lua_rawseti(L, -2, index++);
            }
            return 1;
        }

        int InputGetPinch(lua_State *L)
        {
            float scale = 1.0f;
            glm::vec2 center{0.0f}, pan{0.0f};
            const bool active = TouchControls::GetPinch(scale, center, pan);
            lua_pushnumber(L, active ? scale : 1.0f);
            const glm::vec2 viewport = Input::WindowToViewport(center);
            lua_pushnumber(L, viewport.x);
            lua_pushnumber(L, viewport.y);
            lua_pushnumber(L, active ? pan.x : 0.0f);
            lua_pushnumber(L, active ? pan.y : 0.0f);
            return 5;
        }

        int InputIsTouchDevice(lua_State *L)
        {
            lua_pushboolean(L, TouchControls::IsEnabled());
            return 1;
        }

        const luaL_Reg InputActionFunctions[] = {
            {"SetTouchControls", InputSetTouchControls},
            {"IsTouchDevice", InputIsTouchDevice},
            {"SetFloatingStick", InputSetFloatingStick},
            {"GetTouches", InputGetTouches},
            {"GetPinch", InputGetPinch},
            {"IsActionDown", InputIsActionDown},
            {"IsActionJustPressed", InputIsActionJustPressed},
            {"IsActionJustReleased", InputIsActionJustReleased},
            {"GetVector", InputGetVector},
            {"HasAction", InputHasAction},
            {"SetActionValue", InputSetActionValue},
            {"Rebind", InputRebind},
            {"AddBinding", InputAddBinding},
            {"RemoveBinding", InputRemoveBinding},
            {"ClearBindings", InputClearBindings},
            {"ResetBindings", InputResetBindings},
            {"GetBindings", InputGetBindings},
            {"DescribeBinding", InputDescribeBinding},
            {"GetActions", InputGetActions},
            {"GetPressedBinding", InputGetPressedBinding},
            {"IsGamepadConnected", InputIsGamepadConnected},
            {"GetGamepadAxis", InputGetGamepadAxis},
            {"IsGamepadDown", InputIsGamepadDown},
            {"SetGamepadDeadzone", InputSetGamepadDeadzone},
            {nullptr, nullptr}};
    }

    void RegisterInputActions(lua_State *L)
    {
        lua_getglobal(L, "Input");
        if (lua_istable(L, -1))
            luaL_setfuncs(L, InputActionFunctions, 0);
        lua_pop(L, 1);
    }
}
