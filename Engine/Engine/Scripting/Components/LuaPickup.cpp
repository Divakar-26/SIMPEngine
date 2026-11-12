#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *PickupMeta = "SIMP.Pickup";

        int PickupSetKey(lua_State *L)
        {
            PickupComponent *c = CheckComponent<PickupComponent>(L, PickupMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->key.assign(text, length);
            return 0;
        }
        int PickupGetKey(lua_State *L)
        {
            const PickupComponent *c = CheckComponent<PickupComponent>(L, PickupMeta);
            lua_pushlstring(L, c->key.data(), c->key.size());
            return 1;
        }
        int PickupSetAmount(lua_State *L)
        {
            PickupComponent *c = CheckComponent<PickupComponent>(L, PickupMeta);
            c->amount = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int PickupGetAmount(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<PickupComponent>(L, PickupMeta)->amount);
            return 1;
        }
        int PickupSetSound(lua_State *L)
        {
            PickupComponent *c = CheckComponent<PickupComponent>(L, PickupMeta);
            size_t length = 0;
            const char *text = luaL_optlstring(L, 2, "", &length);
            c->sound.assign(text, length);
            return 0;
        }
        int PickupSetEnabled(lua_State *L)
        {
            PickupComponent *c = CheckComponent<PickupComponent>(L, PickupMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }
        int PickupIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<PickupComponent>(L, PickupMeta)->enabled);
            return 1;
        }
        int PickupSetDestroy(lua_State *L)
        {
            PickupComponent *c = CheckComponent<PickupComponent>(L, PickupMeta);
            luaL_checkany(L, 2);
            c->destroyOnPickup = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg PickupMethods[] = {
            {"SetKey", PickupSetKey},
            {"GetKey", PickupGetKey},
            {"SetAmount", PickupSetAmount},
            {"GetAmount", PickupGetAmount},
            {"SetSound", PickupSetSound},
            {"SetEnabled", PickupSetEnabled},
            {"IsEnabled", PickupIsEnabled},
            {"SetDestroyOnPickup", PickupSetDestroy},
            {nullptr, nullptr}};
    }

    const ComponentBinding &PickupBinding()
    {
        static const ComponentBinding binding{"Pickup", PickupMeta, PickupMethods, &HasOf<PickupComponent>, &AddOf<PickupComponent>, &RemoveOf<PickupComponent>};
        return binding;
    }
}
