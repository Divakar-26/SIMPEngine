#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *DamageMeta = "SIMP.Damage";

        int DamageGetAmount(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<DamageComponent>(L, DamageMeta)->amount);
            return 1;
        }

        int DamageSetAmount(lua_State *L)
        {
            DamageComponent *damage = CheckComponent<DamageComponent>(L, DamageMeta);
            damage->amount = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int DamageGetInterval(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<DamageComponent>(L, DamageMeta)->interval);
            return 1;
        }

        int DamageSetInterval(lua_State *L)
        {
            DamageComponent *damage = CheckComponent<DamageComponent>(L, DamageMeta);
            damage->interval = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int DamageGetKnockback(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<DamageComponent>(L, DamageMeta)->knockback);
            return 1;
        }

        int DamageSetKnockback(lua_State *L)
        {
            DamageComponent *damage = CheckComponent<DamageComponent>(L, DamageMeta);
            damage->knockback = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int DamageGetFilter(lua_State *L)
        {
            const DamageComponent *damage = CheckComponent<DamageComponent>(L, DamageMeta);
            lua_pushlstring(L, damage->filter.data(), damage->filter.size());
            return 1;
        }

        int DamageSetFilter(lua_State *L)
        {
            DamageComponent *damage = CheckComponent<DamageComponent>(L, DamageMeta);
            size_t length = 0;
            const char *filter = luaL_optlstring(L, 2, "", &length);
            damage->filter.assign(filter, length);
            return 0;
        }

        int DamageIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<DamageComponent>(L, DamageMeta)->enabled);
            return 1;
        }

        int DamageSetEnabled(lua_State *L)
        {
            DamageComponent *damage = CheckComponent<DamageComponent>(L, DamageMeta);
            luaL_checkany(L, 2);
            damage->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int DamageSetDestroyOnHit(lua_State *L)
        {
            DamageComponent *damage = CheckComponent<DamageComponent>(L, DamageMeta);
            luaL_checkany(L, 2);
            damage->destroyOnHit = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int DamageReset(lua_State *L)
        {
            CheckComponent<DamageComponent>(L, DamageMeta)->cooldowns.clear();
            return 0;
        }

        const luaL_Reg DamageMethods[] = {
            {"GetAmount", DamageGetAmount},
            {"SetAmount", DamageSetAmount},
            {"GetInterval", DamageGetInterval},
            {"SetInterval", DamageSetInterval},
            {"GetKnockback", DamageGetKnockback},
            {"SetKnockback", DamageSetKnockback},
            {"GetFilter", DamageGetFilter},
            {"SetFilter", DamageSetFilter},
            {"IsEnabled", DamageIsEnabled},
            {"SetEnabled", DamageSetEnabled},
            {"SetDestroyOnHit", DamageSetDestroyOnHit},
            {"Reset", DamageReset},
            {nullptr, nullptr}};
    }

    const ComponentBinding &DamageBinding()
    {
        static const ComponentBinding binding{"Damage", DamageMeta, DamageMethods, &HasOf<DamageComponent>, &AddOf<DamageComponent>, &RemoveOf<DamageComponent>};
        return binding;
    }
}
