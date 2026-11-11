#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *OscillatorMeta = "SIMP.Oscillator";

        int OscillatorPlay(lua_State *L)
        {
            CheckComponent<OscillatorComponent>(L, OscillatorMeta)->playing = true;
            return 0;
        }

        int OscillatorStop(lua_State *L)
        {
            CheckComponent<OscillatorComponent>(L, OscillatorMeta)->playing = false;
            return 0;
        }

        int OscillatorIsPlaying(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<OscillatorComponent>(L, OscillatorMeta)->playing);
            return 1;
        }

        int OscillatorGetMove(lua_State *L)
        {
            const OscillatorComponent *osc = CheckComponent<OscillatorComponent>(L, OscillatorMeta);
            lua_pushnumber(L, osc->move.x);
            lua_pushnumber(L, osc->move.y);
            return 2;
        }

        int OscillatorSetMove(lua_State *L)
        {
            OscillatorComponent *osc = CheckComponent<OscillatorComponent>(L, OscillatorMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            osc->move = {x, y};
            return 0;
        }

        int OscillatorGetFrequency(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<OscillatorComponent>(L, OscillatorMeta)->frequency);
            return 1;
        }

        int OscillatorSetFrequency(lua_State *L)
        {
            OscillatorComponent *osc = CheckComponent<OscillatorComponent>(L, OscillatorMeta);
            osc->frequency = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int OscillatorSetSwing(lua_State *L)
        {
            OscillatorComponent *osc = CheckComponent<OscillatorComponent>(L, OscillatorMeta);
            osc->swing = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int OscillatorSetSpin(lua_State *L)
        {
            OscillatorComponent *osc = CheckComponent<OscillatorComponent>(L, OscillatorMeta);
            osc->spin = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int OscillatorSetPulse(lua_State *L)
        {
            OscillatorComponent *osc = CheckComponent<OscillatorComponent>(L, OscillatorMeta);
            osc->pulse = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        const luaL_Reg OscillatorMethods[] = {
            {"Play", OscillatorPlay},
            {"Stop", OscillatorStop},
            {"IsPlaying", OscillatorIsPlaying},
            {"GetMove", OscillatorGetMove},
            {"SetMove", OscillatorSetMove},
            {"GetFrequency", OscillatorGetFrequency},
            {"SetFrequency", OscillatorSetFrequency},
            {"SetSwing", OscillatorSetSwing},
            {"SetSpin", OscillatorSetSpin},
            {"SetPulse", OscillatorSetPulse},
            {nullptr, nullptr}};
    }

    const ComponentBinding &OscillatorBinding()
    {
        static const ComponentBinding binding{"Oscillator", OscillatorMeta, OscillatorMethods, &HasOf<OscillatorComponent>, &AddOf<OscillatorComponent>, &RemoveOf<OscillatorComponent>};
        return binding;
    }
}
