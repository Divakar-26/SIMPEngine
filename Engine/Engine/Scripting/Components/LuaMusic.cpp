#include "LuaComponents.h"

#include <Engine/Scripting/LuaHelpers.h>
#include <Engine/Scene/Scene.h>

namespace SIMPEngine::LuaBindings
{
    namespace
    {
        constexpr const char *MusicMeta = "SIMP.Music";

        int MusicPlayerPlay(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<MusicComponent>(L, MusicMeta);
            MusicComponent &music = entity->GetComponent<MusicComponent>();
            MusicSystem::Start(music, entity->GetScene()->GetMusicSystem().Random());
            return 0;
        }

        int MusicPlayerNext(lua_State *L)
        {
            Entity *entity = CheckHandleEntity<MusicComponent>(L, MusicMeta);
            MusicComponent &music = entity->GetComponent<MusicComponent>();
            MusicSystem::Next(music, entity->GetScene()->GetMusicSystem().Random());
            return 0;
        }

        int MusicPlayerStop(lua_State *L)
        {
            MusicSystem::Stop(*CheckComponent<MusicComponent>(L, MusicMeta));
            return 0;
        }

        int MusicPlayerGetTrackCount(lua_State *L)
        {
            lua_pushinteger(L, static_cast<lua_Integer>(CheckComponent<MusicComponent>(L, MusicMeta)->tracks.size()));
            return 1;
        }

        int MusicPlayerAddTrack(lua_State *L)
        {
            MusicComponent *c = CheckComponent<MusicComponent>(L, MusicMeta);
            size_t length = 0;
            const char *path = luaL_checklstring(L, 2, &length);
            c->tracks.emplace_back(path, length);
            return 0;
        }

        int MusicPlayerClearTracks(lua_State *L)
        {
            CheckComponent<MusicComponent>(L, MusicMeta)->tracks.clear();
            return 0;
        }

        int MusicPlayerSetVolume(lua_State *L)
        {
            MusicComponent *c = CheckComponent<MusicComponent>(L, MusicMeta);
            c->volume = static_cast<float>(luaL_checknumber(L, 2));
            return 0;
        }
        int MusicPlayerSetShuffle(lua_State *L)
        {
            MusicComponent *c = CheckComponent<MusicComponent>(L, MusicMeta);
            luaL_checkany(L, 2);
            c->shuffle = lua_toboolean(L, 2) != 0;
            return 0;
        }

        const luaL_Reg MusicMethods[] = {
            {"Play", MusicPlayerPlay},
            {"Next", MusicPlayerNext},
            {"Stop", MusicPlayerStop},
            {"AddTrack", MusicPlayerAddTrack},
            {"ClearTracks", MusicPlayerClearTracks},
            {"GetTrackCount", MusicPlayerGetTrackCount},
            {"SetVolume", MusicPlayerSetVolume},
            {"SetShuffle", MusicPlayerSetShuffle},
            {nullptr, nullptr}};
    }

    const ComponentBinding &MusicBinding()
    {
        static const ComponentBinding binding{"Music", MusicMeta, MusicMethods, &HasOf<MusicComponent>, &AddOf<MusicComponent>, &RemoveOf<MusicComponent>};
        return binding;
    }
}
