#include <Engine/Audio/Music.h>

namespace SIMPEngine
{
    namespace
    {
        VoiceId s_Voice = 0;
        std::string s_Path;
        float s_Volume = 1.0f;
        bool s_Paused = false;
    }

    void Music::Play(const std::string &path, float volume, float fadeIn, float fadeOutPrevious, bool loop)
    {
        if (path.empty())
        {
            Stop(fadeOutPrevious);
            return;
        }

        s_Volume = volume;
        if (s_Voice && AudioEngine::IsPlaying(s_Voice) && s_Path == path)
        {
            AudioEngine::SetVolume(s_Voice, volume);
            AudioEngine::SetLooping(s_Voice, loop);
            if (s_Paused)
                Resume();
            return;
        }

        if (s_Voice)
            AudioEngine::FadeTo(s_Voice, 0.0f, fadeOutPrevious, true);

        s_Paused = false;
        s_Voice = AudioEngine::Play(path, volume, 1.0f, loop, AudioBus::Music);
        s_Path = s_Voice ? path : std::string();
        if (s_Voice && fadeIn > 0.0f)
        {
            AudioEngine::SetFade(s_Voice, 0.0f);
            AudioEngine::FadeTo(s_Voice, 1.0f, fadeIn);
        }
    }

    void Music::Stop(float fadeOut)
    {
        if (s_Voice)
            AudioEngine::FadeTo(s_Voice, 0.0f, fadeOut, true);
        s_Voice = 0;
        s_Path.clear();
        s_Paused = false;
    }

    void Music::Pause()
    {
        if (!IsPlaying() || s_Paused)
            return;
        AudioEngine::SetVoicePaused(s_Voice, true);
        s_Paused = true;
    }

    void Music::Resume()
    {
        if (!IsPlaying() || !s_Paused)
            return;
        AudioEngine::SetVoicePaused(s_Voice, false);
        s_Paused = false;
    }

    void Music::SetVolume(float volume)
    {
        s_Volume = volume;
        if (s_Voice)
            AudioEngine::SetVolume(s_Voice, volume);
    }

    bool Music::IsPlaying()
    {
        return s_Voice != 0 && AudioEngine::IsPlaying(s_Voice);
    }

    bool Music::IsPaused()
    {
        return s_Paused && IsPlaying();
    }

    const std::string &Music::Current()
    {
        if (s_Voice && !AudioEngine::IsPlaying(s_Voice))
        {
            s_Voice = 0;
            s_Path.clear();
        }
        return s_Path;
    }

    float Music::GetVolume()
    {
        return s_Volume;
    }
}
