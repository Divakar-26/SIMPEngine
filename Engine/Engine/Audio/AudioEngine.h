#pragma once

#include <cstdint>
#include <string>

namespace SIMPEngine
{
    using VoiceId = uint32_t;

    enum class AudioBus : uint8_t
    {
        Sfx,
        Music,
        Ui,
        Count
    };

    class AudioEngine
    {
    public:
        static bool Init();
        static void Shutdown();
        static void Update();
        static bool IsAvailable();

        static VoiceId Play(const std::string &path, float volume = 1.0f, float pitch = 1.0f, bool loop = false, AudioBus bus = AudioBus::Sfx);
        static void Stop(VoiceId voice);
        static void StopAll();
        static bool IsPlaying(VoiceId voice);
        static void SetVolume(VoiceId voice, float volume);
        static void SetPitch(VoiceId voice, float pitch);
        static void SetLooping(VoiceId voice, bool loop);
        static void SetVoicePaused(VoiceId voice, bool paused);

        static void SetFade(VoiceId voice, float fade);
        static void FadeTo(VoiceId voice, float fade, float seconds, bool stopWhenSilent = false);

        static void SetMasterVolume(float volume);
        static float GetMasterVolume();
        static void SetBusVolume(AudioBus bus, float volume);
        static float GetBusVolume(AudioBus bus);
        static bool BusFromName(const std::string &name, AudioBus &bus);
        static void SetPaused(bool paused);

        static bool Preload(const std::string &path);
        static float GetClipLength(const std::string &path);
        static bool IsSupported(const std::string &path);
        static void ClearCache();
    };
}
