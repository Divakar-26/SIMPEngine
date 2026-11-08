#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *TimerMeta = "SIMP.Timer";

        int TimerStart(lua_State *L)
        {
            CheckComponent<TimerComponent>(L, TimerMeta)->running = true;
            return 0;
        }

        int TimerStop(lua_State *L)
        {
            CheckComponent<TimerComponent>(L, TimerMeta)->running = false;
            return 0;
        }

        int TimerRestart(lua_State *L)
        {
            TimerComponent *timer = CheckComponent<TimerComponent>(L, TimerMeta);
            timer->elapsed = 0.0f;
            timer->running = true;
            return 0;
        }

        int TimerIsRunning(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<TimerComponent>(L, TimerMeta)->running);
            return 1;
        }

        int TimerGetInterval(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<TimerComponent>(L, TimerMeta)->interval);
            return 1;
        }

        int TimerSetInterval(lua_State *L)
        {
            TimerComponent *timer = CheckComponent<TimerComponent>(L, TimerMeta);
            timer->interval = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int TimerIsRepeating(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<TimerComponent>(L, TimerMeta)->repeat);
            return 1;
        }

        int TimerSetRepeat(lua_State *L)
        {
            TimerComponent *timer = CheckComponent<TimerComponent>(L, TimerMeta);
            luaL_checkany(L, 2);
            timer->repeat = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int TimerGetElapsed(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<TimerComponent>(L, TimerMeta)->elapsed);
            return 1;
        }

        int TimerGetRemaining(lua_State *L)
        {
            const TimerComponent *timer = CheckComponent<TimerComponent>(L, TimerMeta);
            const float remaining = timer->interval - timer->elapsed;
            lua_pushnumber(L, remaining > 0.0f ? remaining : 0.0f);
            return 1;
        }

        const luaL_Reg TimerMethods[] = {
            {"Start", TimerStart},
            {"Stop", TimerStop},
            {"Restart", TimerRestart},
            {"IsRunning", TimerIsRunning},
            {"GetInterval", TimerGetInterval},
            {"SetInterval", TimerSetInterval},
            {"IsRepeating", TimerIsRepeating},
            {"SetRepeat", TimerSetRepeat},
            {"GetElapsed", TimerGetElapsed},
            {"GetRemaining", TimerGetRemaining},
            {nullptr, nullptr}};
    }

    const ComponentBinding &TimerBinding()
    {
        static const ComponentBinding binding{"Timer", TimerMeta, TimerMethods, &HasOf<TimerComponent>, &AddOf<TimerComponent>, &RemoveOf<TimerComponent>};
        return binding;
    }
}
