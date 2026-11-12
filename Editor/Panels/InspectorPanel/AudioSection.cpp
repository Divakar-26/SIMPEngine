#include "InspectorPanel.h"

#include <Engine/Audio/AudioEngine.h>
#include <Engine/Audio/Music.h>

#include <cstdio>

namespace
{
    const std::vector<std::string> AudioExtensions = {".wav", ".ogg"};
    SIMPEngine::VoiceId s_PreviewVoice = 0;
    entt::entity s_PreviewEntity = entt::null;

    bool IsAudioFile(const std::string &path)
    {
        const std::string ext = EditorUI::LowerExtension(path);
        return ext == ".wav" || ext == ".ogg";
    }
}

void InspectorPanel::DrawAudioSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    EditorContext &ctx = *m_Context;
    SIMPEngine::Scene *const scene = m_Context->Scene;
    const entt::entity handle = m_SelectedEntity.GetHandle();

    DrawComponentSection<AudioSourceComponent>(ctx, "Audio Source", m_SelectedEntity, [this, &ctx, scene, handle](AudioSourceComponent &audio)
                                               {
        PropertyLabel("Clip");
        if (Button(audio.clip.empty() ? "Choose .wav or .ogg..." : audio.clip.c_str(), ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)) && m_AssetPicker)
        {
            m_AssetPicker->Open("Select Audio Clip", "assets://", [this, scene, handle](const std::string &vpath)
                                {
                if (m_Context->Scene != scene)
                    return;
                SIMPEngine::Entity target(handle, scene);
                if (!target || !target.HasComponent<AudioSourceComponent>())
                    return;
                EditorCommands::RecordEdit(*m_Context);
                target.GetComponent<AudioSourceComponent>().clip = vpath; }, AudioExtensions);
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string path(static_cast<const char *>(payload->Data));
                if (IsAudioFile(path))
                {
                    EditorCommands::RecordEdit(ctx);
                    audio.clip = path;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (!audio.clip.empty())
        {
            char info[64];
            float length = SIMPEngine::AudioEngine::GetClipLength(audio.clip);
            if (length > 0.0f)
                std::snprintf(info, sizeof(info), "%.2f s", length);
            else
                std::snprintf(info, sizeof(info), "Could not load clip");
            PropertyValue("Length", info, length > 0.0f ? &Colors::TextDim : &Colors::Error);
        }

        static const char *buses[] = {"Sfx", "Music", "UI"};
        PropertySegmented("Bus", &audio.bus, buses, 3);
        PropertyFloat("Volume", &audio.volume, 0.01f, 0.0f, 4.0f, "%.2f");
        PropertyFloat("Pitch", &audio.pitch, 0.01f, 0.1f, 4.0f, "%.2f");
        PropertyBool("Loop", &audio.loop);
        PropertyBool("Play On Start", &audio.playOnStart);

        bool previewing = s_PreviewEntity == handle && SIMPEngine::AudioEngine::IsPlaying(s_PreviewVoice);
        if (previewing)
        {
            SIMPEngine::AudioEngine::SetVolume(s_PreviewVoice, audio.volume);
            SIMPEngine::AudioEngine::SetPitch(s_PreviewVoice, audio.pitch);
        }

        PropertyLabel("");
        ImGui::BeginDisabled(audio.clip.empty() || !SIMPEngine::AudioEngine::IsAvailable());
        if (Button(previewing ? "Stop Preview" : "Preview", previewing ? ButtonKind::Primary : ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        {
            SIMPEngine::AudioEngine::Stop(s_PreviewVoice);
            s_PreviewVoice = 0;
            if (!previewing)
            {
                s_PreviewVoice = SIMPEngine::AudioEngine::Play(audio.clip, audio.volume, audio.pitch, audio.loop, static_cast<SIMPEngine::AudioBus>(audio.bus));
                s_PreviewEntity = handle;
            }
        }
        ImGui::EndDisabled();
        if (!SIMPEngine::AudioEngine::IsAvailable())
            PropertyValue("", "No audio device", &Colors::Warning);
        else if (audio.voice)
            PropertyValue("State", "Playing", &Colors::Success); });

    DrawComponentSection<MusicComponent>(ctx, "Music", m_SelectedEntity, [this, &ctx, scene, handle](MusicComponent &music)
                                         {
        SectionLabel("Playlist");
        int remove = -1;
        for (size_t i = 0; i < music.tracks.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            char label[32];
            std::snprintf(label, sizeof(label), "Track %zu", i + 1);
            PropertyLabel(label);
            const float width = ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x;
            if (Button(music.tracks[i].c_str(), ButtonKind::Secondary, ImVec2(width, 0)) && m_AssetPicker)
            {
                m_AssetPicker->Open("Select Music", "assets://", [this, scene, handle, i](const std::string &vpath)
                                    {
                    if (m_Context->Scene != scene)
                        return;
                    SIMPEngine::Entity target(handle, scene);
                    if (!target || !target.HasComponent<MusicComponent>())
                        return;
                    auto &tracks = target.GetComponent<MusicComponent>().tracks;
                    if (i >= tracks.size())
                        return;
                    EditorCommands::RecordEdit(*m_Context);
                    tracks[i] = vpath; }, AudioExtensions);
            }
            ImGui::SameLine();
            if (Button("x", ButtonKind::Ghost, ImVec2(ImGui::GetFrameHeight(), 0)))
                remove = static_cast<int>(i);
            ImGui::PopID();
        }
        if (remove >= 0)
        {
            EditorCommands::RecordEdit(ctx);
            music.tracks.erase(music.tracks.begin() + remove);
        }

        PropertyLabel("");
        if (Button("Add Track...", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)) && m_AssetPicker)
        {
            m_AssetPicker->Open("Add Music Track", "assets://", [this, scene, handle](const std::string &vpath)
                                {
                if (m_Context->Scene != scene)
                    return;
                SIMPEngine::Entity target(handle, scene);
                if (!target || !target.HasComponent<MusicComponent>())
                    return;
                EditorCommands::RecordEdit(*m_Context);
                target.GetComponent<MusicComponent>().tracks.push_back(vpath); }, AudioExtensions);
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string path(static_cast<const char *>(payload->Data));
                if (IsAudioFile(path))
                {
                    EditorCommands::RecordEdit(ctx);
                    music.tracks.push_back(path);
                }
            }
            ImGui::EndDragDropTarget();
        }

        SectionLabel("Playback");
        PropertyFloat("Volume", &music.volume, 0.01f, 0.0f, 2.0f, "%.2f");
        PropertyFloat("Fade In", &music.fadeIn, 0.05f, 0.0f, 60.0f, "%.2f s");
        PropertyFloat("Fade Out", &music.fadeOut, 0.05f, 0.0f, 60.0f, "%.2f s");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("How long the previous music takes to fade when this one starts, so tracks crossfade");
        PropertyBool("Shuffle", &music.shuffle);
        PropertyBool("Loop Playlist", &music.loopPlaylist);
        PropertyBool("Play On Start", &music.playOnStart);

        SectionLabel("Zone");
        PropertyBool("Play On Enter", &music.onTrigger);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Start when something enters this entity's Trigger Zone, for boss rooms and different areas");
        if (music.onTrigger)
        {
            PropertyBool("Stop On Exit", &music.stopOnExit);
            if (!SIMPEngine::Entity(handle, scene).HasComponent<TriggerComponent>())
            {
                PropertyLabel("");
                ImGui::PushStyleColor(ImGuiCol_Text, Colors::Warning);
                ImGui::TextWrapped("Needs a Trigger Zone on this entity");
                ImGui::PopStyleColor();
            }
        }

        const bool previewing = SIMPEngine::Music::IsPlaying() && !music.tracks.empty() && SIMPEngine::Music::Current() == music.tracks.front() && ctx.IsEditing();
        PropertyLabel("");
        ImGui::BeginDisabled(music.tracks.empty() || !SIMPEngine::AudioEngine::IsAvailable() || !ctx.IsEditing());
        if (Button(previewing ? "Stop Preview" : "Preview", previewing ? ButtonKind::Primary : ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        {
            if (previewing)
                SIMPEngine::Music::Stop(0.3f);
            else
                SIMPEngine::Music::Play(music.tracks.front(), music.volume, music.fadeIn, 0.3f, true);
        }
        ImGui::EndDisabled();
        if (!SIMPEngine::AudioEngine::IsAvailable())
            PropertyValue("", "No audio device", &Colors::Warning); });
}
