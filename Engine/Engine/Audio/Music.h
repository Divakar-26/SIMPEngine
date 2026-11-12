#pragma once

#include <Engine/Audio/AudioEngine.h>

#include <string>

namespace SIMPEngine
{
    class Music
    {
    public:
        static void Play(const std::string &path, float volume = 1.0f, float fadeIn = 1.0f, float fadeOutPrevious = 1.0f, bool loop = true);
        static void Stop(float fadeOut = 0.5f);
        static void Pause();
        static void Resume();
        static void SetVolume(float volume);

        static bool IsPlaying();
        static bool IsPaused();
        static const std::string &Current();
        static float GetVolume();
    };
}
