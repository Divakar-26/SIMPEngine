#include "LuaComponents.h"

#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scripting/LuaHelpers.h>

#include <string>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *SpriteMeta = "SIMP.Sprite";

        int SpriteGetTint(lua_State *L)
        {
            return PushRgba(L, CheckComponent<SpriteComponent>(L, SpriteMeta)->tint);
        }

        int SpriteSetTint(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            ReadRgba(L, 2, sprite->tint);
            return 0;
        }

        int SpriteSetTexture(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            size_t length = 0;
            const char *raw = luaL_checklstring(L, 2, &length);
            const bool keepSize = lua_toboolean(L, 3) != 0;
            {
                const std::string path(raw, length);
                sprite->texture = TextureManager::Get().LoadTexture(path, path);
                if (sprite->texture && !keepSize && sprite->texture->GetWidth() > 0)
                {
                    sprite->width = static_cast<float>(sprite->texture->GetWidth());
                    sprite->height = static_cast<float>(sprite->texture->GetHeight());
                }
            }
            return 0;
        }

        int SpriteGetTexture(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            if (!sprite->texture)
            {
                lua_pushnil(L);
                return 1;
            }
            const std::string path = sprite->texture->GetPath();
            lua_pushlstring(L, path.data(), path.size());
            return 1;
        }

        int SpriteGetFlip(lua_State *L)
        {
            const SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            lua_pushboolean(L, sprite->flipX);
            lua_pushboolean(L, sprite->flipY);
            return 2;
        }

        int SpriteSetFlip(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            luaL_checkany(L, 2);
            luaL_checkany(L, 3);
            sprite->flipX = lua_toboolean(L, 2) != 0;
            sprite->flipY = lua_toboolean(L, 3) != 0;
            return 0;
        }

        int SpriteGetSize(lua_State *L)
        {
            const SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            lua_pushnumber(L, sprite->width);
            lua_pushnumber(L, sprite->height);
            return 2;
        }

        int SpriteSetSize(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            const float width = static_cast<float>(luaL_checknumber(L, 2));
            const float height = static_cast<float>(luaL_checknumber(L, 3));
            sprite->width = width;
            sprite->height = height;
            return 0;
        }

        int SpriteSetRegion(lua_State *L)
        {
            SpriteComponent *sprite = CheckComponent<SpriteComponent>(L, SpriteMeta);
            const float x = static_cast<float>(luaL_checknumber(L, 2));
            const float y = static_cast<float>(luaL_checknumber(L, 3));
            const float w = static_cast<float>(luaL_checknumber(L, 4));
            const float h = static_cast<float>(luaL_checknumber(L, 5));
            sprite->region = SDL_FRect{x, y, w, h};
            sprite->useRegion = true;
            return 0;
        }

        int SpriteClearRegion(lua_State *L)
        {
            CheckComponent<SpriteComponent>(L, SpriteMeta)->useRegion = false;
            return 0;
        }

        bool AddSprite(Entity &entity)
        {
            SpriteComponent &sprite = entity.AddComponent<SpriteComponent>();
            sprite.width = 64.0f;
            sprite.height = 64.0f;
            return true;
        }

        const luaL_Reg SpriteMethods[] = {
            {"SetTexture", SpriteSetTexture},
            {"GetTexture", SpriteGetTexture},
            {"GetTint", SpriteGetTint},
            {"SetTint", SpriteSetTint},
            {"GetFlip", SpriteGetFlip},
            {"SetFlip", SpriteSetFlip},
            {"GetSize", SpriteGetSize},
            {"SetSize", SpriteSetSize},
            {"SetRegion", SpriteSetRegion},
            {"ClearRegion", SpriteClearRegion},
            {nullptr, nullptr}};
    }

    const ComponentBinding &SpriteBinding()
    {
        static const ComponentBinding binding{"Sprite", SpriteMeta, SpriteMethods, &HasOf<SpriteComponent>, &AddSprite, &RemoveOf<SpriteComponent>};
        return binding;
    }
}
