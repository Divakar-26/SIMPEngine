#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *PickableMeta = "SIMP.Pickable";

        int PickableIsHeld(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<PickableComponent>(L, PickableMeta)->held);
            return 1;
        }

        int PickableDrop(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<PickableComponent>(L, PickableMeta);
            PickableSystem::Drop(*entity->GetScene(), entity->GetHandle());
            return 0;
        }

        int PickableIsEnabled(lua_State *L)
        {
            lua_pushboolean(L, CheckComponent<PickableComponent>(L, PickableMeta)->enabled);
            return 1;
        }

        int PickableSetEnabled(lua_State *L)
        {
            PickableComponent *c = CheckComponent<PickableComponent>(L, PickableMeta);
            luaL_checkany(L, 2);
            c->enabled = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int PickableSetThrow(lua_State *L)
        {
            PickableComponent *c = CheckComponent<PickableComponent>(L, PickableMeta);
            c->throwStrength = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int PickableSetSmoothing(lua_State *L)
        {
            PickableComponent *c = CheckComponent<PickableComponent>(L, PickableMeta);
            c->smoothing = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        const luaL_Reg PickableMethods[] = {
            {"IsHeld", PickableIsHeld},
            {"Drop", PickableDrop},
            {"IsEnabled", PickableIsEnabled},
            {"SetEnabled", PickableSetEnabled},
            {"SetThrowStrength", PickableSetThrow},
            {"SetSmoothing", PickableSetSmoothing},
            {nullptr, nullptr}};
    }

    const ComponentBinding &PickableBinding()
    {
        static const ComponentBinding binding{"Pickable", PickableMeta, PickableMethods, &HasOf<PickableComponent>, &AddOf<PickableComponent>, &RemoveOf<PickableComponent>};
        return binding;
    }
}
