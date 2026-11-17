#include <Engine/Scene/Systems/PhysicsSystem.h>
#include "LuaHelpers.h"
#include "LuaBindings.h"

#include <new>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <algorithm>

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
        (void)transform;
        PhysicsSystem::SyncBodyToTransform(entity->GetScene()->GetRegistry(), entity->GetHandle());
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
        const int type = lua_type(L, first);
        if (type == LUA_TSTRING || type == LUA_TTABLE)
        {
            SDL_Color value{};
            if (!ReadColorValue(L, first, value))
                luaL_error(L, "bad color '%s': use r, g, b, a?, \"#rrggbb\", {r, g, b, a?} or a name (%s)", type == LUA_TSTRING ? lua_tostring(L, first) : "table", ColorNameList());
            if (lua_isnumber(L, first + 1))
                value.a = ToByte(L, first + 1);
            color = value;
            return;
        }
        const Uint8 r = ToByte(L, first);
        const Uint8 g = ToByte(L, first + 1);
        const Uint8 b = ToByte(L, first + 2);
        const Uint8 a = ToByteOpt(L, first + 3, color.a);
        color = SDL_Color{r, g, b, a};
    }

    void SuggestName(const char *name, const char *const *names, size_t count, char *out, size_t size)
    {
        if (size == 0)
            return;
        out[0] = '\0';
        const size_t n = std::strlen(name);
        const char *best = nullptr;
        size_t bestDistance = std::max<size_t>(2, n / 3) + 1;
        std::vector<size_t> row;
        for (size_t k = 0; k < count; ++k)
        {
            const char *candidate = names[k];
            const size_t m = std::strlen(candidate);
            row.assign(m + 1, 0);
            for (size_t j = 0; j <= m; ++j)
                row[j] = j;
            for (size_t i = 1; i <= n; ++i)
            {
                size_t diagonal = row[0];
                row[0] = i;
                for (size_t j = 1; j <= m; ++j)
                {
                    const size_t above = row[j];
                    const bool same = std::tolower(static_cast<unsigned char>(name[i - 1])) == std::tolower(static_cast<unsigned char>(candidate[j - 1]));
                    row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (same ? 0 : 1)});
                    diagonal = above;
                }
            }
            if (row[m] < bestDistance)
            {
                bestDistance = row[m];
                best = candidate;
            }
        }
        if (best)
            std::snprintf(out, size, " (did you mean '%s'?)", best);
    }
}
