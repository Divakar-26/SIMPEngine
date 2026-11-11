#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *SpawnerMeta = "SIMP.Spawner";

        int SpawnerStart(lua_State *L)
        {
            CheckComponent<SpawnerComponent>(L, SpawnerMeta)->playing = true;
            return 0;
        }

        int SpawnerStop(lua_State *L)
        {
            CheckComponent<SpawnerComponent>(L, SpawnerMeta)->playing = false;
            return 0;
        }

        int SpawnerIsRunning(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<SpawnerComponent>(L, SpawnerMeta)->playing);
            return 1;
        }

        int SpawnerSpawn(lua_State *L)
        {
            SpawnerComponent *spawner = CheckComponent<SpawnerComponent>(L, SpawnerMeta);
            const lua_Integer count = luaL_optinteger(L, 2, 1);
            if (count > 0)
                spawner->pending += static_cast<int>(count < 1000 ? count : 1000);
            return 0;
        }

        int SpawnerGetAlive(lua_State *L)
        {
            lua_pushinteger(L, static_cast<lua_Integer>(CheckComponent<SpawnerComponent>(L, SpawnerMeta)->alive.size()));
            return 1;
        }

        int SpawnerGetSpawned(lua_State *L)
        {
            lua_pushinteger(L, CheckComponent<SpawnerComponent>(L, SpawnerMeta)->spawned);
            return 1;
        }

        int SpawnerGetInterval(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<SpawnerComponent>(L, SpawnerMeta)->interval);
            return 1;
        }

        int SpawnerSetInterval(lua_State *L)
        {
            SpawnerComponent *spawner = CheckComponent<SpawnerComponent>(L, SpawnerMeta);
            spawner->interval = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int SpawnerSetMaxAlive(lua_State *L)
        {
            SpawnerComponent *spawner = CheckComponent<SpawnerComponent>(L, SpawnerMeta);
            spawner->maxAlive = static_cast<int>(luaL_checkinteger(L, 2));
            return 0;
        }

        int SpawnerSetVelocity(lua_State *L)
        {
            SpawnerComponent *spawner = CheckComponent<SpawnerComponent>(L, SpawnerMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            spawner->velocity = {x, y};
            return 0;
        }

        int SpawnerGetPrototype(lua_State *L)
        {
            const SpawnerComponent *spawner = CheckComponent<SpawnerComponent>(L, SpawnerMeta);
            lua_pushlstring(L, spawner->prototype.data(), spawner->prototype.size());
            return 1;
        }

        int SpawnerSetPrototype(lua_State *L)
        {
            SpawnerComponent *spawner = CheckComponent<SpawnerComponent>(L, SpawnerMeta);
            size_t length = 0;
            const char *name = luaL_checklstring(L, 2, &length);
            spawner->prototype.assign(name, length);
            return 0;
        }

        const luaL_Reg SpawnerMethods[] = {
            {"Start", SpawnerStart},
            {"Stop", SpawnerStop},
            {"IsRunning", SpawnerIsRunning},
            {"Spawn", SpawnerSpawn},
            {"GetAlive", SpawnerGetAlive},
            {"GetSpawned", SpawnerGetSpawned},
            {"GetInterval", SpawnerGetInterval},
            {"SetInterval", SpawnerSetInterval},
            {"SetMaxAlive", SpawnerSetMaxAlive},
            {"SetVelocity", SpawnerSetVelocity},
            {"GetPrototype", SpawnerGetPrototype},
            {"SetPrototype", SpawnerSetPrototype},
            {nullptr, nullptr}};
    }

    const ComponentBinding &SpawnerBinding()
    {
        static const ComponentBinding binding{"Spawner", SpawnerMeta, SpawnerMethods, &HasOf<SpawnerComponent>, &AddOf<SpawnerComponent>, &RemoveOf<SpawnerComponent>};
        return binding;
    }
}
