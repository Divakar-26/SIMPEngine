#include "LuaHelpers.h"
#include "LuaBindings.h"

#include <new>

namespace SIMPEngine::LuaBindings
{
    Entity *CheckEntity(lua_State *L, int index)
    {
        return static_cast<Entity *>(luaL_checkudata(L, index, EntityMeta));
    }

    Entity *CheckLiveEntity(lua_State *L, int index)
    {
        Entity *entity = CheckEntity(L, index);
        if (!static_cast<bool>(*entity))
            luaL_error(L, "entity is no longer valid");
        return entity;
    }

    Scene *RequireScene(lua_State *L)
    {
        Scene *scene = GetActiveScene();
        if (!scene)
            luaL_error(L, "no active scene");
        return scene;
    }

    TransformComponent *CheckTransform(lua_State *L, int index, Entity **entityOut)
    {
        Entity *entity = CheckLiveEntity(L, index);
        if (!entity->HasComponent<TransformComponent>())
            luaL_error(L, "entity has no transform");
        if (entityOut)
            *entityOut = entity;
        return &entity->GetComponent<TransformComponent>();
    }

    void SyncBody(Entity *entity, const TransformComponent &transform)
    {
        PhysicsComponent *physics = entity->GetScene()->GetRegistry().try_get<PhysicsComponent>(entity->GetHandle());
        if (!physics || !physics->body)
            return;

        physics->body->position = {transform.position.x, transform.position.y};
        physics->body->orientation = glm::radians(transform.rotation);
        physics->body->calculateDerivativeData();
    }

    entt::entity FindFirstByName(Scene *scene, std::string_view wanted)
    {
        auto view = scene->GetRegistry().view<TagComponent>();
        for (auto handle : view)
        {
            if (view.get<TagComponent>(handle).Tag == wanted)
                return handle;
        }
        return entt::null;
    }

    void PushHandle(lua_State *L, const Entity &entity, const char *meta)
    {
        void *memory = lua_newuserdatauv(L, sizeof(Entity), 0);
        new (memory) Entity(entity);
        luaL_setmetatable(L, meta);
    }

    Uint8 ToByte(lua_State *L, int index)
    {
        const lua_Number value = luaL_checknumber(L, index);
        if (value <= 0.0)
            return 0;
        if (value >= 255.0)
            return 255;
        return static_cast<Uint8>(value);
    }

    Uint8 ToByteOpt(lua_State *L, int index, Uint8 fallback)
    {
        if (lua_isnoneornil(L, index))
            return fallback;
        return ToByte(L, index);
    }

    int PushRgba(lua_State *L, const SDL_Color &color)
    {
        lua_pushinteger(L, color.r);
        lua_pushinteger(L, color.g);
        lua_pushinteger(L, color.b);
        lua_pushinteger(L, color.a);
        return 4;
    }

    void ReadRgba(lua_State *L, int first, SDL_Color &color)
    {
        const Uint8 r = ToByte(L, first);
        const Uint8 g = ToByte(L, first + 1);
        const Uint8 b = ToByte(L, first + 2);
        const Uint8 a = ToByteOpt(L, first + 3, color.a);
        color = SDL_Color{r, g, b, a};
    }
}
