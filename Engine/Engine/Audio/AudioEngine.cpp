#include <Engine/Audio/AudioEngine.h>

#include <unordered_set>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Log.h>

#include <SDL3/SDL.h>

#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_STDIO
extern "C"
{
#include <stb_vorbis.c>
}
#undef STB_VORBIS_HEADER_ONLY

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <utility>

namespace SIMPEngine
{
    namespace
    {
        struct Clip
        {
            SDL_AudioSpec Spec{};
            std::vector<Uint8> Data;
        };

        using FileBytes = std::vector<Uint8>;

        struct Voice
        {
            SDL_AudioStream *Stream = nullptr;
            std::shared_ptr<Clip> Source;
            std::shared_ptr<FileBytes> Encoded;
            stb_vorbis *Decoder = nullptr;
            int Channels = 0;
            int Rate = 0;
            bool Finished = false;
            AudioBus Bus = AudioBus::Sfx;
            float Volume = 1.0f;
            float Fade = 1.0f;
            float FadeTarget = 1.0f;
            float FadeRate = 0.0f;
            bool StopWhenSilent = false;
            bool Loop = false;
        };

        SDL_AudioDeviceID s_Device = 0;
        SDL_AudioSpec s_DeviceSpec{};
        float s_MasterVolume = 1.0f;
        std::array<float, static_cast<size_t>(AudioBus::Count)> s_BusVolume{1.0f, 1.0f, 1.0f};
        bool s_Paused = false;
        Uint64 s_LastTicks = 0;
        VoiceId s_NextVoice = 1;
        std::unordered_map<std::string, std::shared_ptr<Clip>> s_Clips;
        std::unordered_map<std::string, std::shared_ptr<FileBytes>> s_Files;
        std::unordered_map<VoiceId, Voice> s_Voices;

        std::string Extension(const std::string &path)
        {
            const size_t dot = path.find_last_of('.');
            if (dot == std::string::npos)
                return {};
            std::string ext = path.substr(dot);
            for (char &c : ext)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return ext;
        }

        bool IsOgg(const std::string &path)
        {
            return Extension(path) == ".ogg";
        }

        std::string RealPath(const std::string &path)
        {
            if (auto resolved = VFS::Resolve(path))
                return *resolved;
            return path;
        }

        std::unordered_set<std::string> s_ReportedMissing;

        bool ReportOnce(const std::string &path)
        {
            return s_ReportedMissing.insert(path).second;
        }

        std::shared_ptr<Clip> LoadClip(const std::string &path)
        {
            if (auto it = s_Clips.find(path); it != s_Clips.end())
                return it->second;

            SDL_AudioSpec spec;
            Uint8 *buffer = nullptr;
            Uint32 length = 0;
            if (!SDL_LoadWAV(RealPath(path).c_str(), &spec, &buffer, &length))
            {
                if (ReportOnce(path))
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

        std::shared_ptr<FileBytes> LoadFileBytes(const std::string &path)
        {
            if (auto it = s_Files.find(path); it != s_Files.end())
                return it->second;

            size_t size = 0;
            void *data = SDL_LoadFile(RealPath(path).c_str(), &size);
            if (!data)
            {
                if (ReportOnce(path))
                    CORE_ERROR("Audio: cannot read '{}': {}", path, SDL_GetError());
                return nullptr;
            }
            auto bytes = std::make_shared<FileBytes>(static_cast<Uint8 *>(data), static_cast<Uint8 *>(data) + size);
            SDL_free(data);
            s_Files[path] = bytes;
            return bytes;
        }

        float Gain(const Voice &voice)
        {
            return std::max(0.0f, voice.Volume) * std::max(0.0f, voice.Fade) * s_BusVolume[static_cast<size_t>(voice.Bus)] * s_MasterVolume;
        }

        void ApplyGain(Voice &voice)
        {
            if (voice.Stream)
                SDL_SetAudioStreamGain(voice.Stream, Gain(voice));
        }

        void Destroy(Voice &voice)
        {
            if (voice.Stream)
                SDL_DestroyAudioStream(voice.Stream);
            voice.Stream = nullptr;
            if (voice.Decoder)
                stb_vorbis_close(voice.Decoder);
            voice.Decoder = nullptr;
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

        void Stream(Voice &voice)
        {
            if (!voice.Stream || !voice.Decoder || voice.Finished)
                return;

            const int wanted = voice.Rate * voice.Channels * static_cast<int>(sizeof(float)) / 2;
            std::vector<float> chunk(4096 * static_cast<size_t>(voice.Channels));
            bool restarted = false;
            while (SDL_GetAudioStreamQueued(voice.Stream) < wanted)
            {
                const int frames = stb_vorbis_get_samples_float_interleaved(voice.Decoder, voice.Channels, chunk.data(), static_cast<int>(chunk.size()));
                if (frames <= 0)
                {
                    if (voice.Loop && !restarted && stb_vorbis_seek_start(voice.Decoder))
                    {
                        restarted = true;
                        continue;
                    }
                    voice.Finished = true;
                    SDL_FlushAudioStream(voice.Stream);
                    return;
                }
                restarted = false;
                SDL_PutAudioStreamData(voice.Stream, chunk.data(), frames * voice.Channels * static_cast<int>(sizeof(float)));
            }
        }

        bool Ended(const Voice &voice)
        {
            if (!voice.Stream)
                return true;
            if (voice.Decoder && !voice.Finished)
                return false;
            if (voice.Loop && !voice.Decoder)
                return false;
            return SDL_GetAudioStreamQueued(voice.Stream) <= 0 && SDL_GetAudioStreamAvailable(voice.Stream) <= 0;
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
        s_Files.clear();
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
        const Uint64 now = SDL_GetTicksNS();
        float dt = s_LastTicks ? static_cast<float>(static_cast<double>(now - s_LastTicks) / 1.0e9) : 0.0f;
        s_LastTicks = now;
        dt = s_Paused ? 0.0f : std::min(dt, 0.25f);

        for (auto it = s_Voices.begin(); it != s_Voices.end();)
        {
            Voice &voice = it->second;

            if (voice.FadeRate > 0.0f)
            {
                const float step = voice.FadeRate * dt;
                if (std::abs(voice.FadeTarget - voice.Fade) <= step)
                {
                    voice.Fade = voice.FadeTarget;
                    voice.FadeRate = 0.0f;
                }
                else
                    voice.Fade += voice.Fade < voice.FadeTarget ? step : -step;
                ApplyGain(voice);

                if (voice.StopWhenSilent && voice.FadeRate == 0.0f && voice.Fade <= 0.0001f)
                {
                    Destroy(voice);
                    it = s_Voices.erase(it);
                    continue;
                }
            }

            if (voice.Decoder)
                Stream(voice);
            else if (voice.Loop)
                Feed(voice);

            if (Ended(voice))
            {
                Destroy(voice);
                it = s_Voices.erase(it);
                continue;
            }
            ++it;
        }
    }

    VoiceId AudioEngine::Play(const std::string &path, float volume, float pitch, bool loop, AudioBus bus)
    {
        if (!s_Device || path.empty())
            return 0;

        Voice voice;
        SDL_AudioSpec source{};

        if (IsOgg(path))
        {
            voice.Encoded = LoadFileBytes(path);
            if (!voice.Encoded || voice.Encoded->empty())
                return 0;
            int error = 0;
            voice.Decoder = stb_vorbis_open_memory(voice.Encoded->data(), static_cast<int>(voice.Encoded->size()), &error, nullptr);
            if (!voice.Decoder)
            {
                CORE_ERROR("Audio: '{}' is not a valid OGG Vorbis file (error {})", path, error);
                return 0;
            }
            const stb_vorbis_info info = stb_vorbis_get_info(voice.Decoder);
            voice.Channels = info.channels;
            voice.Rate = static_cast<int>(info.sample_rate);
            source.format = SDL_AUDIO_F32;
            source.channels = info.channels;
            source.freq = static_cast<int>(info.sample_rate);
        }
        else
        {
            voice.Source = LoadClip(path);
            if (!voice.Source || voice.Source->Data.empty())
                return 0;
            source = voice.Source->Spec;
        }

        voice.Stream = SDL_CreateAudioStream(&source, &s_DeviceSpec);
        if (!voice.Stream)
        {
            CORE_ERROR("Audio: cannot create stream: {}", SDL_GetError());
            Destroy(voice);
            return 0;
        }

        voice.Bus = bus;
        voice.Volume = volume;
        voice.Loop = loop;
        ApplyGain(voice);
        SDL_SetAudioStreamFrequencyRatio(voice.Stream, std::clamp(pitch, 0.01f, 100.0f));

        if (voice.Decoder)
            Stream(voice);
        else
        {
            SDL_PutAudioStreamData(voice.Stream, voice.Source->Data.data(), static_cast<int>(voice.Source->Data.size()));
            if (loop)
                Feed(voice);
            else
                SDL_FlushAudioStream(voice.Stream);
        }

        if (!SDL_BindAudioStream(s_Device, voice.Stream))
        {
            CORE_ERROR("Audio: cannot bind stream: {}", SDL_GetError());
            Destroy(voice);
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
        ApplyGain(it->second);
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
        if (!it->second.Decoder && !loop)
            SDL_FlushAudioStream(it->second.Stream);
    }

    void AudioEngine::SetVoicePaused(VoiceId id, bool paused)
    {
        auto it = s_Voices.find(id);
        if (it == s_Voices.end() || !it->second.Stream)
            return;
        if (paused)
            SDL_PauseAudioStreamDevice(it->second.Stream);
        else
            SDL_ResumeAudioStreamDevice(it->second.Stream);
    }

    void AudioEngine::SetFade(VoiceId id, float fade)
    {
        auto it = s_Voices.find(id);
        if (it == s_Voices.end())
            return;
        it->second.Fade = it->second.FadeTarget = std::max(0.0f, fade);
        it->second.FadeRate = 0.0f;
        ApplyGain(it->second);
    }

    void AudioEngine::FadeTo(VoiceId id, float fade, float seconds, bool stopWhenSilent)
    {
        auto it = s_Voices.find(id);
        if (it == s_Voices.end())
            return;
        Voice &voice = it->second;
        fade = std::max(0.0f, fade);
        voice.StopWhenSilent = stopWhenSilent;
        if (seconds <= 0.0f)
        {
            SetFade(id, fade);
            if (stopWhenSilent && fade <= 0.0001f)
                Stop(id);
            return;
        }
        voice.FadeTarget = fade;
        voice.FadeRate = std::max(std::abs(fade - voice.Fade), 0.0001f) / seconds;
    }

    void AudioEngine::SetMasterVolume(float volume)
    {
        s_MasterVolume = std::max(0.0f, volume);
        for (auto &[id, voice] : s_Voices)
            ApplyGain(voice);
    }

    float AudioEngine::GetMasterVolume()
    {
        return s_MasterVolume;
    }

    void AudioEngine::SetBusVolume(AudioBus bus, float volume)
    {
        s_BusVolume[static_cast<size_t>(bus)] = std::max(0.0f, volume);
        for (auto &[id, voice] : s_Voices)
            if (voice.Bus == bus)
                ApplyGain(voice);
    }

    float AudioEngine::GetBusVolume(AudioBus bus)
    {
        return s_BusVolume[static_cast<size_t>(bus)];
    }

    bool AudioEngine::BusFromName(const std::string &name, AudioBus &bus)
    {
        std::string lower = name;
        for (char &c : lower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower == "sfx" || lower == "sound")
            bus = AudioBus::Sfx;
        else if (lower == "music")
            bus = AudioBus::Music;
        else if (lower == "ui")
            bus = AudioBus::Ui;
        else
            return false;
        return true;
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
        if (IsOgg(path))
            return LoadFileBytes(path) != nullptr;
        return LoadClip(path) != nullptr;
    }

    float AudioEngine::GetClipLength(const std::string &path)
    {
        if (IsOgg(path))
        {
            auto bytes = LoadFileBytes(path);
            if (!bytes || bytes->empty())
                return 0.0f;
            int error = 0;
            stb_vorbis *decoder = stb_vorbis_open_memory(bytes->data(), static_cast<int>(bytes->size()), &error, nullptr);
            if (!decoder)
                return 0.0f;
            const float seconds = stb_vorbis_stream_length_in_seconds(decoder);
            stb_vorbis_close(decoder);
            return seconds;
        }

        auto clip = LoadClip(path);
        if (!clip)
            return 0.0f;
        int frameSize = SDL_AUDIO_FRAMESIZE(clip->Spec);
        if (frameSize <= 0 || clip->Spec.freq <= 0)
            return 0.0f;
        return static_cast<float>(clip->Data.size()) / static_cast<float>(frameSize) / static_cast<float>(clip->Spec.freq);
    }

    bool AudioEngine::IsSupported(const std::string &path)
    {
        const std::string ext = Extension(path);
        return ext == ".wav" || ext == ".ogg";
    }

    void AudioEngine::ClearCache()
    {
        s_Clips.clear();
        s_Files.clear();
    }
}
