#include "LuaModules.h"

#include "LuaBindings.h"
#include "LuaHelpers.h"
#include "LuaInclude.h"

#include <Engine/Animation/MotionPresets.h>
#include <Engine/Scene/Scene.h>

#include <string>
#include <vector>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        float Number(lua_State *L, int index, const char *key, float fallback)
        {
            lua_getfield(L, index, key);
            const float value = lua_isnumber(L, -1) ? static_cast<float>(lua_tonumber(L, -1)) : fallback;
            lua_pop(L, 1);
            return value;
        }

        bool Color(lua_State *L, int index, const char *key, glm::vec4 &out)
        {
            lua_getfield(L, index, key);
            SDL_Color c{};
            const bool ok = !lua_isnil(L, -1) && ReadColorValue(L, lua_gettop(L), c);
            lua_pop(L, 1);
            if (ok)
                out = glm::vec4(c.r, c.g, c.b, c.a) / 255.0f;
            return ok;
        }

        void ReadPoints(lua_State *L, int index, std::vector<glm::vec2> &out)
        {
            const lua_Integer n = luaL_len(L, index);
            lua_rawgeti(L, index, 1);
            const bool nested = lua_istable(L, -1);
            lua_pop(L, 1);
            if (nested)
            {
                for (lua_Integer i = 1; i <= n; ++i)
                {
                    lua_rawgeti(L, index, i);
                    if (lua_istable(L, -1))
                    {
                        lua_rawgeti(L, -1, 1);
                        lua_rawgeti(L, -2, 2);
                        if (lua_isnumber(L, -2) && lua_isnumber(L, -1))
                            out.emplace_back(static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1)));
                        else
                        {
                            lua_pop(L, 2);
                            lua_getfield(L, -1, "x");
                            lua_getfield(L, -2, "y");
                            if (lua_isnumber(L, -2) && lua_isnumber(L, -1))
                                out.emplace_back(static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1)));
                        }
                        lua_pop(L, 2);
                    }
                    lua_pop(L, 1);
                }
                return;
            }
            for (lua_Integer i = 1; i + 1 <= n; i += 2)
            {
                lua_rawgeti(L, index, i);
                lua_rawgeti(L, index, i + 1);
                if (lua_isnumber(L, -2) && lua_isnumber(L, -1))
                    out.emplace_back(static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1)));
                lua_pop(L, 2);
            }
        }

        bool ReadOptions(lua_State *L, int index, MotionOptions &o, std::string &error)
        {
            if (!lua_istable(L, index))
                return true;
            o.Position.x = Number(L, index, "x", 0.0f);
            o.Position.y = Number(L, index, "y", 0.0f);
            lua_getfield(L, index, "at");
            if (lua_istable(L, -1))
            {
                lua_rawgeti(L, -1, 1);
                lua_rawgeti(L, -2, 2);
                o.Position = {static_cast<float>(lua_tonumber(L, -2)), static_cast<float>(lua_tonumber(L, -1))};
                lua_pop(L, 2);
            }
            lua_pop(L, 1);
            o.Size = Number(L, index, "size", 0.0f);
            o.Duration = Number(L, index, "duration", Number(L, index, "time", 0.0f));
            o.Delay = Number(L, index, "delay", 0.0f);
            o.Width = Number(L, index, "width", 0.0f);
            o.Z = Number(L, index, "z", 0.0f);
            o.Hold = Number(L, index, "hold", -1.0f);
            Color(L, index, "color", o.Color);
            Color(L, index, "accent", o.Accent);

            lua_getfield(L, index, "text");
            if (lua_isstring(L, -1))
                o.Text = lua_tostring(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, index, "points");
            if (lua_istable(L, -1))
                ReadPoints(L, lua_gettop(L), o.Points);
            lua_pop(L, 1);

            lua_getfield(L, index, "values");
            if (lua_istable(L, -1))
            {
                const lua_Integer n = luaL_len(L, -1);
                for (lua_Integer i = 1; i <= n; ++i)
                {
                    lua_rawgeti(L, -1, i);
                    if (lua_isnumber(L, -1))
                        o.Values.push_back(static_cast<float>(lua_tonumber(L, -1)));
                    lua_pop(L, 1);
                }
            }
            else if (lua_isnumber(L, -1))
                o.Values.push_back(static_cast<float>(lua_tonumber(L, -1)));
            lua_pop(L, 1);

            lua_getfield(L, index, "fn");
            if (lua_isfunction(L, -1))
            {
                const int fn = lua_gettop(L);
                float from = -6.2831853f, to = 6.2831853f;
                lua_getfield(L, index, "range");
                if (lua_istable(L, -1))
                {
                    lua_rawgeti(L, -1, 1);
                    lua_rawgeti(L, -2, 2);
                    from = static_cast<float>(lua_tonumber(L, -2));
                    to = static_cast<float>(lua_tonumber(L, -1));
                    lua_pop(L, 2);
                }
                lua_pop(L, 1);
                const int samples = static_cast<int>(Number(L, index, "samples", 96.0f));
                o.Values.clear();
                for (int i = 0; i < std::max(2, samples); ++i)
                {
                    const float x = from + (to - from) * static_cast<float>(i) / static_cast<float>(std::max(2, samples) - 1);
                    lua_pushvalue(L, fn);
                    lua_pushnumber(L, x);
                    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
                    {
                        error = lua_tostring(L, -1) ? lua_tostring(L, -1) : "error in fn";
                        lua_pop(L, 2);
                        return false;
                    }
                    o.Values.push_back(lua_isnumber(L, -1) ? static_cast<float>(lua_tonumber(L, -1)) : 0.0f);
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);

            lua_getfield(L, index, "parent");
            if (Entity *parent = static_cast<Entity *>(luaL_testudata(L, -1, EntityMeta)))
                o.Parent = parent->GetHandle();
            lua_pop(L, 1);
            return true;
        }

        int MotionPlay(lua_State *L)
        {
            size_t length = 0;
            const char *rawName = luaL_checklstring(L, 1, &length);
            Scene *scene = GetActiveScene();
            if (!scene)
                return luaL_error(L, "Motion.Play: no active scene (call it from a script hook)");
            bool known = false;
            bool failed = false;
            char message[256] = {};
            entt::entity root = entt::null;
            {
                const MotionPreset *preset = MotionPresets::Find(std::string(rawName, length));
                if (preset)
                {
                    known = true;
                    MotionOptions options;
                    std::string error;
                    if (!ReadOptions(L, 2, options, error))
                    {
                        failed = true;
                        std::snprintf(message, sizeof(message), "%s", error.c_str());
                    }
                    else
                        root = MotionPresets::Play(*scene, *preset, options);
                }
            }
            if (!known)
                return luaL_error(L, "Motion.Play: unknown preset '%s' (see Motion.Names())", rawName);
            if (failed)
                return luaL_error(L, "Motion.Play: %s", message);
            PushEntity(L, Entity(root, scene));
            return 1;
        }

        int MotionNames(lua_State *L)
        {
            const char *category = luaL_optstring(L, 1, "");
            lua_newtable(L);
            lua_Integer n = 0;
            for (const MotionPreset &preset : MotionPresets::All())
            {
                if (category[0] && preset.Category != category)
                    continue;
                lua_pushlstring(L, preset.Name.data(), preset.Name.size());
                lua_rawseti(L, -2, ++n);
            }
            return 1;
        }

        int MotionCategories(lua_State *L)
        {
            lua_newtable(L);
            lua_Integer n = 0;
            for (const std::string &category : MotionPresets::Categories())
            {
                lua_pushlstring(L, category.data(), category.size());
                lua_rawseti(L, -2, ++n);
            }
            return 1;
        }

        int MotionInfo(lua_State *L)
        {
            size_t length = 0;
            const char *rawName = luaL_checklstring(L, 1, &length);
            const MotionPreset *preset = MotionPresets::Find(std::string(rawName, length));
            if (!preset)
            {
                lua_pushnil(L);
                return 1;
            }
            static const char *inputs[] = {"none", "points", "values", "text"};
            lua_newtable(L);
            lua_pushlstring(L, preset->Name.data(), preset->Name.size());
            lua_setfield(L, -2, "name");
            lua_pushlstring(L, preset->Category.data(), preset->Category.size());
            lua_setfield(L, -2, "category");
            lua_pushlstring(L, preset->Description.data(), preset->Description.size());
            lua_setfield(L, -2, "description");
            lua_pushnumber(L, preset->Duration);
            lua_setfield(L, -2, "duration");
            lua_pushstring(L, inputs[static_cast<int>(preset->Input)]);
            lua_setfield(L, -2, "input");
            return 1;
        }

        const luaL_Reg MotionFunctions[] = {
            {"Play", MotionPlay},
            {"Names", MotionNames},
            {"Categories", MotionCategories},
            {"Info", MotionInfo},
            {nullptr, nullptr}};
    }

    void RegisterMotion(lua_State *L)
    {
        luaL_newlib(L, MotionFunctions);
        lua_setglobal(L, "Motion");
    }
}
