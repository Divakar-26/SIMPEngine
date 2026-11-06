#include <Engine/Audio/AudioEngine.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Log.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

namespace SIMPEngine
{
    namespace
    {
        struct Clip
        {
            SDL_AudioSpec Spec{};
            std::vector<Uint8> Data;
        };

        struct Voice
        {
            SDL_AudioStream *Stream = nullptr;
            std::shared_ptr<Clip> Source;
            float Volume = 1.0f;
            bool Loop = false;
        };

        SDL_AudioDeviceID s_Device = 0;
        SDL_AudioSpec s_DeviceSpec{};
        float s_MasterVolume = 1.0f;
        bool s_Paused = false;
        VoiceId s_NextVoice = 1;
        std::unordered_map<std::string, std::shared_ptr<Clip>> s_Clips;
        std::unordered_map<VoiceId, Voice> s_Voices;

        std::shared_ptr<Clip> LoadClip(const std::string &path)
        {
            if (auto it = s_Clips.find(path); it != s_Clips.end())
                return it->second;

            std::string real = path;
            if (auto resolved = VFS::Resolve(path))
                real = *resolved;

            SDL_AudioSpec spec;
            Uint8 *buffer = nullptr;
            Uint32 length = 0;
            if (!SDL_LoadWAV(real.c_str(), &spec, &buffer, &length))
            {
                CORE_ERROR("Audio: cannot load '{}': {}", path, SDL_GetError());
                return nullptr;
            }

            auto clip = std::make_shared<Clip>();
            clip->Spec = spec;
            clip->Data.assign(buffer, buffer + length);
            SDL_free(buffer);
            s_Clips[path] = clip;
            return clip;
        }

        void Destroy(Voice &voice)
        {
            if (voice.Stream)
                SDL_DestroyAudioStream(voice.Stream);
            voice.Stream = nullptr;
        }

        void Feed(Voice &voice)
        {
            if (!voice.Stream || !voice.Source || voice.Source->Data.empty())
                return;
            int size = static_cast<int>(voice.Source->Data.size());
            while (SDL_GetAudioStreamQueued(voice.Stream) < size)
                if (!SDL_PutAudioStreamData(voice.Stream, voice.Source->Data.data(), size))
                    break;
        }
    }

    bool AudioEngine::Init()
    {
        if (s_Device)
            return true;

        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            CORE_WARN("Audio: SDL audio unavailable: {}", SDL_GetError());
            return false;
        }

        s_Device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!s_Device)
        {
            CORE_WARN("Audio: no playback device: {}", SDL_GetError());
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            return false;
        }

        int frames = 0;
        SDL_GetAudioDeviceFormat(s_Device, &s_DeviceSpec, &frames);
        CORE_INFO("Audio: {} Hz, {} channels", s_DeviceSpec.freq, s_DeviceSpec.channels);
        return true;
    }

    void AudioEngine::Shutdown()
    {
        StopAll();
        s_Clips.clear();
        if (s_Device)
        {
            SDL_CloseAudioDevice(s_Device);
            s_Device = 0;
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
    }

    bool AudioEngine::IsAvailable()
    {
        return s_Device != 0;
    }

    void AudioEngine::Update()
    {
        for (auto it = s_Voices.begin(); it != s_Voices.end();)
        {
            Voice &voice = it->second;
            if (voice.Loop)
            {
                Feed(voice);
                ++it;
                continue;
            }

            if (!voice.Stream || (SDL_GetAudioStreamQueued(voice.Stream) <= 0 && SDL_GetAudioStreamAvailable(voice.Stream) <= 0))
            {
                Destroy(voice);
                it = s_Voices.erase(it);
                continue;
            }
            ++it;
        }
    }

    VoiceId AudioEngine::Play(const std::string &path, float volume, float pitch, bool loop)
    {
        if (!s_Device || path.empty())
            return 0;

        auto clip = LoadClip(path);
        if (!clip || clip->Data.empty())
            return 0;

        SDL_AudioStream *stream = SDL_CreateAudioStream(&clip->Spec, &s_DeviceSpec);
        if (!stream)
        {
            CORE_ERROR("Audio: cannot create stream: {}", SDL_GetError());
            return 0;
        }

        SDL_SetAudioStreamGain(stream, std::max(0.0f, volume) * s_MasterVolume);
        SDL_SetAudioStreamFrequencyRatio(stream, std::clamp(pitch, 0.01f, 100.0f));

        Voice voice;
        voice.Stream = stream;
        voice.Source = clip;
        voice.Volume = volume;
        voice.Loop = loop;

        SDL_PutAudioStreamData(stream, clip->Data.data(), static_cast<int>(clip->Data.size()));
        if (loop)
            Feed(voice);
        else
            SDL_FlushAudioStream(stream);

        if (!SDL_BindAudioStream(s_Device, stream))
        {
            CORE_ERROR("Audio: cannot bind stream: {}", SDL_GetError());
            SDL_DestroyAudioStream(stream);
            return 0;
        }

        VoiceId id = s_NextVoice++;
        if (s_NextVoice == 0)
            s_NextVoice = 1;
        s_Voices.emplace(id, std::move(voice));
        return id;
    }

    void AudioEngine::Stop(VoiceId id)
    {
        auto it = s_Voices.find(id);
        if (it == s_Voices.end())
            return;
        Destroy(it->second);
        s_Voices.erase(it);
    }

    void AudioEngine::StopAll()
    {
        for (auto &[id, voice] : s_Voices)
            Destroy(voice);
        s_Voices.clear();
    }

    bool AudioEngine::IsPlaying(VoiceId id)
    {
        return s_Voices.find(id) != s_Voices.end();
    }

    void AudioEngine::SetVolume(VoiceId id, float volume)
    {
        auto it = s_Voices.find(id);
        if (it == s_Voices.end() || it->second.Volume == volume)
            return;
        it->second.Volume = volume;
        SDL_SetAudioStreamGain(it->second.Stream, std::max(0.0f, volume) * s_MasterVolume);
    }

    void AudioEngine::SetPitch(VoiceId id, float pitch)
    {
        auto it = s_Voices.find(id);
        if (it != s_Voices.end())
            SDL_SetAudioStreamFrequencyRatio(it->second.Stream, std::clamp(pitch, 0.01f, 100.0f));
    }

    void AudioEngine::SetLooping(VoiceId id, bool loop)
    {
        auto it = s_Voices.find(id);
        if (it == s_Voices.end() || it->second.Loop == loop)
            return;
        it->second.Loop = loop;
        if (!loop)
            SDL_FlushAudioStream(it->second.Stream);
    }

    void AudioEngine::SetMasterVolume(float volume)
    {
        s_MasterVolume = std::max(0.0f, volume);
        for (auto &[id, voice] : s_Voices)
            SDL_SetAudioStreamGain(voice.Stream, std::max(0.0f, voice.Volume) * s_MasterVolume);
    }

    float AudioEngine::GetMasterVolume()
    {
        return s_MasterVolume;
    }

    void AudioEngine::SetPaused(bool paused)
    {
        if (!s_Device || paused == s_Paused)
            return;
        s_Paused = paused;
        if (paused)
            SDL_PauseAudioDevice(s_Device);
        else
            SDL_ResumeAudioDevice(s_Device);
    }

    bool AudioEngine::Preload(const std::string &path)
    {
        return LoadClip(path) != nullptr;
    }

    float AudioEngine::GetClipLength(const std::string &path)
    {
        auto clip = LoadClip(path);
        if (!clip)
            return 0.0f;
        int frameSize = SDL_AUDIO_FRAMESIZE(clip->Spec);
        if (frameSize <= 0 || clip->Spec.freq <= 0)
            return 0.0f;
        return static_cast<float>(clip->Data.size()) / static_cast<float>(frameSize) / static_cast<float>(clip->Spec.freq);
    }

    void AudioEngine::ClearCache()
    {
        s_Clips.clear();
    }
}
