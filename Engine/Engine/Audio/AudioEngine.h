#pragma once

#include <cstdint>
#include <string>

namespace SIMPEngine
{
    using VoiceId = uint32_t;

    class AudioEngine
    {
    public:
        static bool Init();
        static void Shutdown();
        static void Update();
        static bool IsAvailable();

        static VoiceId Play(const std::string &path, float volume = 1.0f, float pitch = 1.0f, bool loop = false);
        static void Stop(VoiceId voice);
        static void StopAll();
        static bool IsPlaying(VoiceId voice);
        static void SetVolume(VoiceId voice, float volume);
        static void SetPitch(VoiceId voice, float pitch);
        static void SetLooping(VoiceId voice, bool loop);

        static void SetMasterVolume(float volume);
        static float GetMasterVolume();
        static void SetPaused(bool paused);

        static bool Preload(const std::string &path);
        static float GetClipLength(const std::string &path);
        static void ClearCache();
    };
}
