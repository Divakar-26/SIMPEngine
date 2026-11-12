#include "LuaModules.h"

#include "LuaHelpers.h"

#include <Engine/Animation/EffectPresets.h>
#include <Engine/Animation/Tween.h>

#include <string>
#include <vector>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        void ReadOptions(lua_State *L, int index, EffectOptions &options)
        {
            if (lua_type(L, index) != LUA_TTABLE)
                return;
            auto number = [&](const char *key, float &out)
            {
                lua_getfield(L, index, key);
                if (lua_isnumber(L, -1))
                    out = static_cast<float>(lua_tonumber(L, -1));
                lua_pop(L, 1);
            };
            number("time", options.Duration);
            number("duration", options.Duration);
            number("delay", options.Delay);
            number("distance", options.Distance);
            number("amount", options.Amount);
            number("speed", options.Speed);
            lua_getfield(L, index, "loop");
            if (lua_type(L, -1) == LUA_TBOOLEAN)
                options.Loop = lua_toboolean(L, -1) ? -1 : 0;
            else if (lua_isnumber(L, -1))
                options.Loop = static_cast<int>(lua_tointeger(L, -1));
            lua_pop(L, 1);
        }

        int EffectPlay(lua_State *L)
        {
            Entity *entity = CheckLiveEntity(L, 1);
            size_t length = 0;
            const char *rawName = luaL_checklstring(L, 2, &length);
            EffectOptions options;
            ReadOptions(L, 3, options);

            uint32_t id = 0;
            bool known = false;
            uint64_t serial = 0;
            {
                const std::string name(rawName, length);
                if (const EffectPreset *preset = EffectPresets::Find(name))
                {
                    known = true;
                    TweenSystem &system = entity->GetScene()->GetTweens();
                    serial = system.Serial();
                    id = EffectPresets::Play(system, entity->GetHandle(), *preset, options);
                }
            }
            if (!known)
                return luaL_error(L, "Effect.Play: unknown effect '%s' (see Effect.Names())", rawName);
            if (id == 0)
                return luaL_error(L, "Effect.Play: could not build '%s'", rawName);
            return PushTween(L, serial, id);
        }

        int EffectNames(lua_State *L)
        {
            const char *category = luaL_optstring(L, 1, "");
            lua_newtable(L);
            lua_Integer n = 0;
            for (const EffectPreset &preset : EffectPresets::All())
            {
                if (category[0] && preset.Category != category)
                    continue;
                lua_pushlstring(L, preset.Name.data(), preset.Name.size());
                lua_rawseti(L, -2, ++n);
            }
            return 1;
        }

        int EffectCategories(lua_State *L)
        {
            lua_newtable(L);
            lua_Integer n = 0;
            for (const std::string &category : EffectPresets::Categories())
            {
                lua_pushlstring(L, category.data(), category.size());
                lua_rawseti(L, -2, ++n);
            }
            return 1;
        }

        int EffectInfo(lua_State *L)
        {
            size_t length = 0;
            const char *rawName = luaL_checklstring(L, 1, &length);
            const EffectPreset *preset = EffectPresets::Find(std::string(rawName, length));
            if (!preset)
            {
                lua_pushnil(L);
                return 1;
            }
            lua_newtable(L);
            auto text = [&](const char *key, const std::string &value)
            {
                lua_pushlstring(L, value.data(), value.size());
                lua_setfield(L, -2, key);
            };
            text("name", preset->Name);
            text("label", preset->Label);
            text("category", preset->Category);
            text("description", preset->Description);
            lua_pushnumber(L, preset->Duration);
            lua_setfield(L, -2, "duration");
            lua_pushboolean(L, preset->Loop);
            lua_setfield(L, -2, "loop");
            return 1;
        }

        const luaL_Reg EffectFunctions[] = {
            {"Play", EffectPlay},
            {"Names", EffectNames},
            {"Categories", EffectCategories},
            {"Info", EffectInfo},
            {nullptr, nullptr}};
    }

    void RegisterEffects(lua_State *L)
    {
        luaL_newlib(L, EffectFunctions);
        lua_setglobal(L, "Effect");
    }
}
