#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>
#include <Engine/Audio/AudioEngine.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *AudioMeta = "SIMP.Audio";

        int AudioSourcePlay(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<AudioSourceComponent>(L, AudioMeta);
            AudioSourceComponent &source = entity->GetComponent<AudioSourceComponent>();
            AudioSystem::StopSource(entity->GetScene()->GetRegistry(), entity->GetHandle());
            source.started = true;
            if (!source.clip.empty())
                source.voice = AudioEngine::Play(source.clip, source.volume, source.pitch, source.loop);
            return 0;
        }

        int AudioSourceStop(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<AudioSourceComponent>(L, AudioMeta);
            AudioSystem::StopSource(entity->GetScene()->GetRegistry(), entity->GetHandle());
            return 0;
        }

        int AudioSourceIsPlaying(lua_State *L)
        {
            const AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            lua_pushboolean(L, source->voice != 0 && AudioEngine::IsPlaying(source->voice));
            return 1;
        }

        int AudioSourceGetVolume(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<AudioSourceComponent>(L, AudioMeta)->volume);
            return 1;
        }

        int AudioSourceSetVolume(lua_State *L)
        {
            AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            source->volume = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int AudioSourceGetPitch(lua_State *L)
        {
            lua_pushnumber(L, CheckComponent<AudioSourceComponent>(L, AudioMeta)->pitch);
            return 1;
        }

        int AudioSourceSetPitch(lua_State *L)
        {
            AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            source->pitch = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }

        int AudioSourceSetLoop(lua_State *L)
        {
            AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            luaL_checkany(L, 2);
            source->loop = lua_toboolean(L, 2) != 0;
            return 0;
        }

        int AudioSourceSetClip(lua_State *L)
        {
            AudioSourceComponent *source = CheckComponent<AudioSourceComponent>(L, AudioMeta);
            size_t length = 0;
            const char *clip = luaL_checklstring(L, 2, &length);
            source->clip.assign(clip, length);
            return 0;
        }

        const luaL_Reg AudioMethods[] = {
            {"Play", AudioSourcePlay},
            {"Stop", AudioSourceStop},
            {"IsPlaying", AudioSourceIsPlaying},
            {"GetVolume", AudioSourceGetVolume},
            {"SetVolume", AudioSourceSetVolume},
            {"GetPitch", AudioSourceGetPitch},
            {"SetPitch", AudioSourceSetPitch},
            {"SetLoop", AudioSourceSetLoop},
            {"SetClip", AudioSourceSetClip},
            {nullptr, nullptr}};
    }

    const ComponentBinding &AudioSourceBinding()
    {
        static const ComponentBinding binding{"Audio", AudioMeta, AudioMethods, &HasOf<AudioSourceComponent>, &AddOf<AudioSourceComponent>, &RemoveOf<AudioSourceComponent>};
        return binding;
    }
}
