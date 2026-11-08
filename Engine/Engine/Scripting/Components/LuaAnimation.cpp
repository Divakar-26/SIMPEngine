#include "LuaComponents.h"

#include <Engine/Rendering/Animation.h>
#include <Engine/Rendering/Texture.h>
#include <Engine/Rendering/TextureManager.h>
#include <Engine/Scripting/LuaHelpers.h>

#include <algorithm>
#include <memory>
#include <string>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *AnimationMeta = "SIMP.Animation";

        Animation *ClipOf(AnimatedSpriteComponent *animation)
        {
            return animation->animation.get();
        }

        int AnimationSetSheet(lua_State *L)
        {
            AnimatedSpriteComponent *animation = CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta);
            size_t length = 0;
            const char *path = luaL_checklstring(L, 2, &length);
            const int frameWidth = static_cast<int>(luaL_checkinteger(L, 3));
            const int frameHeight = static_cast<int>(luaL_checkinteger(L, 4));
            const int frameCount = static_cast<int>(luaL_checkinteger(L, 5));
            const float frameDuration = static_cast<float>(luaL_optnumber(L, 6, 0.1));
            const bool loop = lua_isnoneornil(L, 7) ? true : lua_toboolean(L, 7) != 0;
            if (frameWidth <= 0 || frameHeight <= 0 || frameCount <= 0)
                return luaL_error(L, "frame width, height and count must be positive");

            bool loaded = false;
            {
                const std::string file(path, length);
                auto texture = TextureManager::Get().LoadTexture(file, file);
                if (texture)
                {
                    animation->animation = std::make_unique<Animation>(texture, frameWidth, frameHeight, frameCount, std::max(0.001f, frameDuration), loop);
                    loaded = true;
                }
            }
            lua_pushboolean(L, loaded);
            return 1;
        }

        int AnimationGetSheet(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            if (!clip || !clip->GetTexture())
            {
                lua_pushnil(L);
                return 1;
            }
            const std::string &path = clip->GetTexture()->GetVFSPath();
            lua_pushlstring(L, path.data(), path.size());
            return 1;
        }

        int AnimationPlay(lua_State *L)
        {
            AnimatedSpriteComponent *animation = CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta);
            animation->playing = true;
            if (Animation *clip = ClipOf(animation); clip && clip->IsFinished())
                clip->Reset();
            return 0;
        }

        int AnimationPause(lua_State *L)
        {
            CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta)->playing = false;
            return 0;
        }

        int AnimationIsPlaying(lua_State *L)
        {
            AnimatedSpriteComponent *animation = CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta);
            Animation *clip = ClipOf(animation);
            lua_pushboolean(L, animation->playing && clip && !clip->IsFinished());
            return 1;
        }

        int AnimationRestart(lua_State *L)
        {
            AnimatedSpriteComponent *animation = CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta);
            animation->playing = true;
            if (Animation *clip = ClipOf(animation))
                clip->Reset();
            return 0;
        }

        int AnimationGetFrame(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            lua_pushinteger(L, clip ? clip->GetCurrentFrame() + 1 : 0);
            return 1;
        }

        int AnimationSetFrame(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            const int frame = static_cast<int>(luaL_checkinteger(L, 2));
            if (clip)
                clip->SetPlaybackState(frame - 1, 0.0f, false);
            return 0;
        }

        int AnimationGetFrameCount(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            lua_pushinteger(L, clip ? clip->GetFrameNumber() : 0);
            return 1;
        }

        int AnimationGetFrameSize(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            lua_pushinteger(L, clip ? clip->GetFrameWidth() : 0);
            lua_pushinteger(L, clip ? clip->GetFrameHeight() : 0);
            return 2;
        }

        int AnimationGetFrameDuration(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            lua_pushnumber(L, clip ? clip->GetFrameDuration() : 0.0f);
            return 1;
        }

        int AnimationSetFrameDuration(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            const float duration = static_cast<float>(luaL_checknumber(L, 2));
            if (clip)
                clip->GetFrameDurationRef() = std::max(0.001f, duration);
            return 0;
        }

        int AnimationIsLooping(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            lua_pushboolean(L, clip && clip->IsLooping());
            return 1;
        }

        int AnimationSetLoop(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            const bool loop = lua_toboolean(L, 2) != 0;
            if (clip)
                clip->SetLooping(loop);
            return 0;
        }

        int AnimationIsFinished(lua_State *L)
        {
            Animation *clip = ClipOf(CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta));
            lua_pushboolean(L, clip && clip->IsFinished());
            return 1;
        }

        int AnimationGetSpeed(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta)->speed);
            return 1;
        }

        int AnimationSetSpeed(lua_State *L)
        {
            AnimatedSpriteComponent *animation = CheckComponent<AnimatedSpriteComponent>(L, AnimationMeta);
            animation->speed = std::max(0.0f, static_cast<float>(luaL_checknumber(L, 2)));
            return 0;
        }

        const luaL_Reg AnimationMethods[] = {
            {"SetSheet", AnimationSetSheet},
            {"GetSheet", AnimationGetSheet},
            {"Play", AnimationPlay},
            {"Pause", AnimationPause},
            {"IsPlaying", AnimationIsPlaying},
            {"Restart", AnimationRestart},
            {"GetFrame", AnimationGetFrame},
            {"SetFrame", AnimationSetFrame},
            {"GetFrameCount", AnimationGetFrameCount},
            {"GetFrameSize", AnimationGetFrameSize},
            {"GetFrameDuration", AnimationGetFrameDuration},
            {"SetFrameDuration", AnimationSetFrameDuration},
            {"IsLooping", AnimationIsLooping},
            {"SetLoop", AnimationSetLoop},
            {"IsFinished", AnimationIsFinished},
            {"GetSpeed", AnimationGetSpeed},
            {"SetSpeed", AnimationSetSpeed},
            {nullptr, nullptr}};
    }

    const ComponentBinding &AnimationBinding()
    {
        static const ComponentBinding binding{"Animation", AnimationMeta, AnimationMethods, &HasOf<AnimatedSpriteComponent>, &AddOf<AnimatedSpriteComponent>, &RemoveOf<AnimatedSpriteComponent>};
        return binding;
    }
}
