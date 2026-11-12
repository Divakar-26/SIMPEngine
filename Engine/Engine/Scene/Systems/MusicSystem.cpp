#include "MusicSystem.h"

#include <Engine/Audio/Music.h>
#include <Engine/Scene/Scene.h>
#include <string>

namespace SIMPEngine
{
    namespace
    {
        bool IsOurs(const MusicComponent &music)
        {
            if (music.index < 0 || music.index >= static_cast<int>(music.tracks.size()))
                return false;
            return Music::Current() == music.tracks[static_cast<size_t>(music.index)];
        }

        void PlayIndex(MusicComponent &music, int index)
        {
            music.index = index;
            music.active = true;
            const bool single = music.tracks.size() == 1 && music.loopPlaylist;
            const std::string &track = music.tracks[static_cast<size_t>(index)];
            Music::Play(track, music.volume, music.fadeIn, music.fadeOut, single);
            if (Music::Current() != track)
                music.active = false;
        }
    }

    void MusicSystem::Start(MusicComponent &music, std::mt19937 &random)
    {
        if (music.tracks.empty())
            return;
        const int count = static_cast<int>(music.tracks.size());
        PlayIndex(music, music.shuffle ? static_cast<int>(random() % static_cast<unsigned>(count)) : 0);
    }

    void MusicSystem::Next(MusicComponent &music, std::mt19937 &random)
    {
        if (music.tracks.empty())
            return;
        const int count = static_cast<int>(music.tracks.size());
        int next = music.index + 1;
        if (music.shuffle && count > 1)
        {
            do
                next = static_cast<int>(random() % static_cast<unsigned>(count));
            while (next == music.index);
        }
        else if (next >= count)
        {
            if (!music.loopPlaylist)
            {
                music.active = false;
                return;
            }
            next = 0;
        }
        PlayIndex(music, next);
    }

    void MusicSystem::Stop(MusicComponent &music)
    {
        if (IsOurs(music))
            Music::Stop(music.fadeOut);
        music.active = false;
    }

    void MusicSystem::Update(Scene &scene)
    {
        entt::registry &registry = scene.GetRegistry();
        for (entt::entity handle : registry.view<MusicComponent>())
        {
            MusicComponent &music = registry.get<MusicComponent>(handle);

            if (music.onTrigger)
            {
                const TriggerComponent *trigger = registry.try_get<TriggerComponent>(handle);
                const bool inside = trigger && !trigger->inside.empty();
                if (inside && !music.wasInside)
                    Start(music, m_Random);
                else if (!inside && music.wasInside && music.stopOnExit)
                    Stop(music);
                music.wasInside = inside;
            }
            else if (!music.started)
            {
                if (music.playOnStart)
                    Start(music, m_Random);
            }
            music.started = true;

            if (music.active && !IsOurs(music) && !Music::IsPlaying() && !Music::IsPaused())
                Next(music, m_Random);
            else if (music.active && !IsOurs(music) && Music::IsPlaying())
                music.active = false;
        }
    }
}
