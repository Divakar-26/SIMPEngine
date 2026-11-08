#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *PathMeta = "SIMP.Path";

        int PathPlay(lua_State *L)
        {
            CheckComponent<PathFollowerComponent>(L, PathMeta)->playing = true;
            return 0;
        }

        int PathPause(lua_State *L)
        {
            CheckComponent<PathFollowerComponent>(L, PathMeta)->playing = false;
            return 0;
        }

        int PathRestart(lua_State *L)
        {
            PathFollowerSystem::Restart(*CheckComponent<PathFollowerComponent>(L, PathMeta));
            return 0;
        }

        int PathIsPlaying(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<PathFollowerComponent>(L, PathMeta)->playing);
            return 1;
        }

        int PathGetSpeed(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<PathFollowerComponent>(L, PathMeta)->speed);
            return 1;
        }

        int PathSetSpeed(lua_State *L)
        {
            PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            path->speed = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int PathAddPoint(lua_State *L)
        {
            PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            path->points.emplace_back(x, y);
            return 0;
        }

        int PathClearPoints(lua_State *L)
        {
            PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            path->points.clear();
            path->target = 0;
            return 0;
        }

        int PathGetPointCount(lua_State *L)
        {
            lua_pushinteger(L, static_cast<lua_Integer>(CheckComponent<PathFollowerComponent>(L, PathMeta)->points.size()));
            return 1;
        }

        int PathGetPoint(lua_State *L)
        {
            const PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            const lua_Integer index = luaL_checkinteger(L, 2);
            if (index < 1 || static_cast<size_t>(index) > path->points.size())
                return luaL_error(L, "path point %d is out of range", static_cast<int>(index));
            const glm::vec2 &point = path->points[static_cast<size_t>(index - 1)];
            lua_pushnumber(L, point.x);
            lua_pushnumber(L, point.y);
            return 2;
        }

        int PathSetPoint(lua_State *L)
        {
            PathFollowerComponent *path = CheckComponent<PathFollowerComponent>(L, PathMeta);
            const lua_Integer index = luaL_checkinteger(L, 2);
            const float x = static_cast<float>(luaL_checknumber(L, 3));
            const float y = static_cast<float>(luaL_checknumber(L, 4));
            if (index < 1 || static_cast<size_t>(index) > path->points.size())
                return luaL_error(L, "path point %d is out of range", static_cast<int>(index));
            path->points[static_cast<size_t>(index - 1)] = {x, y};
            return 0;
        }

        const luaL_Reg PathMethods[] = {
            {"Play", PathPlay},
            {"Pause", PathPause},
            {"Restart", PathRestart},
            {"IsPlaying", PathIsPlaying},
            {"GetSpeed", PathGetSpeed},
            {"SetSpeed", PathSetSpeed},
            {"AddPoint", PathAddPoint},
            {"ClearPoints", PathClearPoints},
            {"GetPointCount", PathGetPointCount},
            {"GetPoint", PathGetPoint},
            {"SetPoint", PathSetPoint},
            {nullptr, nullptr}};
    }

    const ComponentBinding &PathBinding()
    {
        static const ComponentBinding binding{"Path", PathMeta, PathMethods, &HasOf<PathFollowerComponent>, &AddOf<PathFollowerComponent>, &RemoveOf<PathFollowerComponent>};
        return binding;
    }
}
