#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *HealthMeta = "SIMP.Health";

        int HealthGetCurrent(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<HealthComponent>(L, HealthMeta)->current);
            return 1;
        }

        int HealthSetCurrent(lua_State *L)
        {
            HealthComponent *health = CheckComponent<HealthComponent>(L, HealthMeta);
            const float value = static_cast<float>(luaL_checknumber(L, 2));
            health->current = value < 0.0f ? 0.0f : (value > health->max ? health->max : value);
            if (health->current > 0.0f)
                health->dead = false;
            return 0;
        }

        int HealthGetMax(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<HealthComponent>(L, HealthMeta)->max);
            return 1;
        }

        int HealthSetMax(lua_State *L)
        {
            HealthComponent *health = CheckComponent<HealthComponent>(L, HealthMeta);
            const float value = static_cast<float>(luaL_checknumber(L, 2));
            health->max = value < 0.0f ? 0.0f : value;
            if (health->current > health->max)
                health->current = health->max;
            return 0;
        }

        int HealthGetFraction(lua_State *L)
        {
            const HealthComponent *health = CheckComponent<HealthComponent>(L, HealthMeta);
            lua_pushnumber(L, health->max > 0.0f ? health->current / health->max : 0.0f);
            return 1;
        }

        int HealthIsDead(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<HealthComponent>(L, HealthMeta)->dead);
            return 1;
        }

        int HealthIsInvulnerable(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<HealthComponent>(L, HealthMeta)->invulnerableRemaining > 0.0f);
            return 1;
        }

        int HealthGetInvulnerability(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<HealthComponent>(L, HealthMeta)->invulnerability);
            return 1;
        }

        int HealthSetInvulnerability(lua_State *L)
        {
            HealthComponent *health = CheckComponent<HealthComponent>(L, HealthMeta);
            const float value = static_cast<float>(luaL_checknumber(L, 2));
            health->invulnerability = value < 0.0f ? 0.0f : value;
            return 0;
        }

        int HealthDamage(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<HealthComponent>(L, HealthMeta);
            const float amount = static_cast<float>(luaL_checknumber(L, 2));
            entt::entity source = entt::null;
            if (!lua_isnoneornil(L, 3))
                source = CheckLiveEntity(L, 3)->GetHandle();
            lua_pushboolean(L, HealthSystem::Damage(*entity->GetScene(), entity->GetHandle(), amount, source));
            return 1;
        }

        int HealthHeal(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<HealthComponent>(L, HealthMeta);
            const float amount = static_cast<float>(luaL_checknumber(L, 2));
            lua_pushnumber(L, HealthSystem::Heal(*entity->GetScene(), entity->GetHandle(), amount));
            return 1;
        }

        int HealthKill(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<HealthComponent>(L, HealthMeta);
            entt::entity source = entt::null;
            if (!lua_isnoneornil(L, 2))
                source = CheckLiveEntity(L, 2)->GetHandle();
            lua_pushboolean(L, HealthSystem::Kill(*entity->GetScene(), entity->GetHandle(), source));
            return 1;
        }

        int HealthRevive(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<HealthComponent>(L, HealthMeta);
            lua_pushboolean(L, HealthSystem::Revive(*entity->GetScene(), entity->GetHandle()));
            return 1;
        }

        const luaL_Reg HealthMethods[] = {
            {"GetCurrent", HealthGetCurrent},
            {"SetCurrent", HealthSetCurrent},
            {"GetMax", HealthGetMax},
            {"SetMax", HealthSetMax},
            {"GetFraction", HealthGetFraction},
            {"IsDead", HealthIsDead},
            {"IsInvulnerable", HealthIsInvulnerable},
            {"GetInvulnerability", HealthGetInvulnerability},
            {"SetInvulnerability", HealthSetInvulnerability},
            {"Damage", HealthDamage},
            {"Heal", HealthHeal},
            {"Kill", HealthKill},
            {"Revive", HealthRevive},
            {nullptr, nullptr}};
    }

    const ComponentBinding &HealthBinding()
    {
        static const ComponentBinding binding{"Health", HealthMeta, HealthMethods, &HasOf<HealthComponent>, &AddOf<HealthComponent>, &RemoveOf<HealthComponent>};
        return binding;
    }
}
