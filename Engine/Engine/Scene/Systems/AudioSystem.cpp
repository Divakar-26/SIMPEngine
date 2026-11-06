#include "AudioSystem.h"
#include <Engine/Audio/AudioEngine.h>
#include <Engine/Scene/Component.h>

namespace SIMPEngine
{
    void AudioSystem::Update(entt::registry &registry)
    {
        auto view = registry.view<AudioSourceComponent>();
        for (auto entity : view)
        {
            auto &source = view.get<AudioSourceComponent>(entity);
            if (!source.started)
            {
                source.started = true;
                if (source.playOnStart && !source.clip.empty())
                    source.voice = AudioEngine::Play(source.clip, source.volume, source.pitch, source.loop);
                continue;
            }

            if (!source.voice)
                continue;
            if (!AudioEngine::IsPlaying(source.voice))
            {
                source.voice = 0;
                continue;
            }
            AudioEngine::SetVolume(source.voice, source.volume);
            AudioEngine::SetPitch(source.voice, source.pitch);
            AudioEngine::SetLooping(source.voice, source.loop);
        }
    }

    void AudioSystem::StopSource(entt::registry &registry, entt::entity entity)
    {
        auto &source = registry.get<AudioSourceComponent>(entity);
        if (source.voice)
            AudioEngine::Stop(source.voice);
        source.voice = 0;
    }
}
