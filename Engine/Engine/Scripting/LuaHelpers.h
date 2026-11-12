#pragma once

#include "LuaBindings.h"
#include "LuaInclude.h"

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>

#include <SDL3/SDL_pixels.h>

#include <string>
#include <string_view>

namespace SIMPEngine::LuaBindings
{
    inline constexpr const char *EntityMeta = "SIMP.Entity";

    Entity *CheckEntity(lua_State *L, int index);
    Entity *CheckLiveEntity(lua_State *L, int index);
    Scene *RequireScene(lua_State *L);
    TransformComponent *CheckTransform(lua_State *L, int index, Entity **entityOut = nullptr);
    void SyncBody(Entity *entity, const TransformComponent &transform);
    entt::entity FindFirstByName(Scene *scene, std::string_view wanted);

    void PushHandle(lua_State *L, const Entity &entity, const char *meta);
    Uint8 ToByte(lua_State *L, int index);
    Uint8 ToByteOpt(lua_State *L, int index, Uint8 fallback);
    int PushRgba(lua_State *L, const SDL_Color &color);
    void ReadRgba(lua_State *L, int first, SDL_Color &color);
    bool ParseHexColor(const char *text, Uint8 out[4]);
    bool ReadColorValue(lua_State *L, int index, SDL_Color &out);
    bool LookupColorName(const char *name, SDL_Color &out);
    const char *ColorNameList();

    template <typename T>
    Entity *CheckHandleEntity(lua_State *L, const char *meta)
    {
        Entity *entity = static_cast<Entity *>(luaL_checkudata(L, 1, meta));
        if (!static_cast<bool>(*entity))
            luaL_error(L, "entity is no longer valid");
        if (!entity->HasComponent<T>())
            luaL_error(L, "entity no longer has this component");
        return entity;
    }

    template <typename T>
    T *CheckComponent(lua_State *L, const char *meta)
    {
        Entity *entity = CheckHandleEntity<T>(L, meta);
        return &entity->GetComponent<T>();
    }

    struct ComponentBinding
    {
        const char *Name;
        const char *Meta;
        const luaL_Reg *Methods;
        bool (*Has)(Entity &);
        bool (*Add)(Entity &);
        void (*Remove)(Entity &);
    };

    template <typename T>
    bool HasOf(Entity &entity)
    {
        return entity.HasComponent<T>();
    }

    template <typename T>
    bool AddOf(Entity &entity)
    {
        entity.AddComponent<T>();
        return true;
    }

    template <typename T>
    void RemoveOf(Entity &entity)
    {
        entity.RemoveComponent<T>();
    }
}
