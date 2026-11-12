#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *CheckpointMeta = "SIMP.Checkpoint";

        int CheckpointSetOffset(lua_State *L)
        {
            CheckpointComponent *c = CheckComponent<CheckpointComponent>(L, CheckpointMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            c->offset = {x, y};
            return 0;
        }
        int CheckpointGetOffset(lua_State *L)
        {
            const CheckpointComponent *c = CheckComponent<CheckpointComponent>(L, CheckpointMeta);
            lua_pushnumber(L, c->offset.x);
            lua_pushnumber(L, c->offset.y);
            return 2;
        }
        int CheckpointSetSound(lua_State *L)
        {
            CheckpointComponent *c = CheckComponent<CheckpointComponent>(L, CheckpointMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->sound.assign(text, length);
            return 0;
        }
        int CheckpointSetEnabled(lua_State *L)
        {
            CheckpointComponent *c = CheckComponent<CheckpointComponent>(L, CheckpointMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int CheckpointIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<CheckpointComponent>(L, CheckpointMeta)->enabled);
            return 1;
        }

        const luaL_Reg CheckpointMethods[] = {
            {"SetOffset", CheckpointSetOffset},
            {"GetOffset", CheckpointGetOffset},
            {"SetSound", CheckpointSetSound},
            {"SetEnabled", CheckpointSetEnabled},
            {"IsEnabled", CheckpointIsEnabled},
            {nullptr, nullptr}};
    }

    const ComponentBinding &CheckpointBinding()
    {
        static const ComponentBinding binding{"Checkpoint", CheckpointMeta, CheckpointMethods, &HasOf<CheckpointComponent>, &AddOf<CheckpointComponent>, &RemoveOf<CheckpointComponent>};
        return binding;
    }
}
