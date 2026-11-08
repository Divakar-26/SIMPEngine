#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *TriggerMeta = "SIMP.Trigger";

        int TriggerGetSize(lua_State *L)
        {
            const TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            lua_pushnumber(L, trigger->width);
            lua_pushnumber(L, trigger->height);
            return 2;
        }

        int TriggerSetSize(lua_State *L)
        {
            TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            const float width = static_cast<float>(luaL_checknumber(L, 2));
            const float height = static_cast<float>(luaL_checknumber(L, 3));
            trigger->width = width;
            trigger->height = height;
            return 0;
        }

        int TriggerGetOffset(lua_State *L)
        {
            const TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            lua_pushnumber(L, trigger->offsetX);
            lua_pushnumber(L, trigger->offsetY);
            return 2;
        }

        int TriggerSetOffset(lua_State *L)
        {
            TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            trigger->offsetX = x;
            trigger->offsetY = y;
            return 0;
        }

        int TriggerGetFilter(lua_State *L)
        {
            const TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            lua_pushlstring(L, trigger->filter.data(), trigger->filter.size());
            return 1;
        }

        int TriggerSetFilter(lua_State *L)
        {
            TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            size_t length = 0;
            const char *filter = luaL_optlstring(L, 2, "", &length);
            trigger->filter.assign(filter, length);
            return 0;
        }

        int TriggerIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<TriggerComponent>(L, TriggerMeta)->enabled);
            return 1;
        }

        int TriggerSetEnabled(lua_State *L)
        {
            TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            luaL_checkany(L, 2);
            trigger->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int TriggerGetInside(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<TriggerComponent>(L, TriggerMeta);
            Scene *scene = entity->GetScene();
            auto &registry = scene->GetRegistry();
            const TriggerComponent &trigger = entity->GetComponent<TriggerComponent>();

            lua_newtable(L);
            lua_Integer index = 1;
            for (entt::entity other : trigger.inside)
            {
                if (!registry.valid(other))
                    continue;
                PushEntity(L, Entity(other, scene));
                lua_rawseti(L, -2, index++);
            }
            return 1;
        }

        int TriggerContains(lua_State *L)
        {
            const TriggerComponent *trigger = CheckComponent<TriggerComponent>(L, TriggerMeta);
            const Entity *other = CheckEntity(L, 2);
            bool found = false;
            for (entt::entity inside : trigger->inside)
            {
                if (inside == other->GetHandle())
                {
                    found = true;
                    break;
                }
            }
            lua_pushboolean(L, found);
            return 1;
        }

        const luaL_Reg TriggerMethods[] = {
            {"GetSize", TriggerGetSize},
            {"SetSize", TriggerSetSize},
            {"GetOffset", TriggerGetOffset},
            {"SetOffset", TriggerSetOffset},
            {"GetFilter", TriggerGetFilter},
            {"SetFilter", TriggerSetFilter},
            {"IsEnabled", TriggerIsEnabled},
            {"SetEnabled", TriggerSetEnabled},
            {"GetInside", TriggerGetInside},
            {"Contains", TriggerContains},
            {nullptr, nullptr}};
    }

    const ComponentBinding &TriggerBinding()
    {
        static const ComponentBinding binding{"Trigger", TriggerMeta, TriggerMethods, &HasOf<TriggerComponent>, &AddOf<TriggerComponent>, &RemoveOf<TriggerComponent>};
        return binding;
    }
}
