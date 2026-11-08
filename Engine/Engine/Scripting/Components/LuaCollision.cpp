#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>
#include <Engine/Math/EntityUtility.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *CollisionMeta = "SIMP.Collision";

        int CollisionGetSize(lua_State *L)
        {
            const CollisionComponent *collision = CheckComponent<CollisionComponent>(L, CollisionMeta);
            lua_pushnumber(L, collision->width);
            lua_pushnumber(L, collision->height);
            return 2;
        }

        int CollisionSetSize(lua_State *L)
        {
            CollisionComponent *collision = CheckComponent<CollisionComponent>(L, CollisionMeta);
            const float width = static_cast<float>(luaL_checknumber(L, 2));
            const float height = static_cast<float>(luaL_checknumber(L, 3));
            collision->width = width;
            collision->height = height;
            return 0;
        }

        int CollisionGetOffset(lua_State *L)
        {
            const CollisionComponent *collision = CheckComponent<CollisionComponent>(L, CollisionMeta);
            lua_pushnumber(L, collision->offsetX);
            lua_pushnumber(L, collision->offsetY);
            return 2;
        }

        int CollisionSetOffset(lua_State *L)
        {
            CollisionComponent *collision = CheckComponent<CollisionComponent>(L, CollisionMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            collision->offsetX = x;
            collision->offsetY = y;
            return 0;
        }

        int CollisionGetBounds(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<CollisionComponent>(L, CollisionMeta);
            const CollisionComponent &collision = entity->GetComponent<CollisionComponent>();
            if (!entity->HasComponent<TransformComponent>())
                return luaL_error(L, "entity has no transform");
            const WorldTransform2D world = DecomposeWorld(entity->GetComponent<TransformComponent>());
            lua_pushnumber(L, world.Position.x + collision.offsetX);
            lua_pushnumber(L, world.Position.y + collision.offsetY);
            lua_pushnumber(L, collision.width * std::abs(world.Scale.x));
            lua_pushnumber(L, collision.height * std::abs(world.Scale.y));
            return 4;
        }

        const luaL_Reg CollisionMethods[] = {
            {"GetSize", CollisionGetSize},
            {"SetSize", CollisionSetSize},
            {"GetOffset", CollisionGetOffset},
            {"SetOffset", CollisionSetOffset},
            {"GetBounds", CollisionGetBounds},
            {nullptr, nullptr}};
    }

    const ComponentBinding &CollisionBinding()
    {
        static const ComponentBinding binding{"Collision", CollisionMeta, CollisionMethods, &HasOf<CollisionComponent>, &AddOf<CollisionComponent>, &RemoveOf<CollisionComponent>};
        return binding;
    }
}
