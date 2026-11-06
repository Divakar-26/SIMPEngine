#include "InspectorPanel.h"
#include "../ParticlePickerPanel.h"

#include <Engine/Particles/ParticleEmitter.h>
#include <Engine/Particles/ParticleSerializer.h>
#include <Engine/Scene/Systems/ParticleSystem.h>

#include <imgui_internal.h>

#include <cmath>
#include <cstdio>

namespace
{
    const std::vector<std::string> ImageExtensions = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif", ".webp"};
    std::string s_SavedPath;
    entt::entity s_SavedEntity = entt::null;

    ImU32 ToU32(const glm::vec4 &c)
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(c.r, c.g, c.b, c.a));
    }

    bool PropertyColorVec4(const char *label, glm::vec4 &color)
    {
        float rgba[4] = {color.r, color.g, color.b, color.a};
        if (!EditorUI::PropertyColor(label, rgba))
            return false;
        color = {rgba[0], rgba[1], rgba[2], rgba[3]};
        return true;
    }

    void DrawGradient(const SIMPEngine::ParticleEffect &e)
    {
        using namespace EditorUI;

        PropertyLabel("");
        float width = ImGui::GetContentRegionAvail().x;
        float height = 14.0f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 max(pos.x + width, pos.y + height);
        ImGui::Dummy(ImVec2(width, height));

        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(pos, max, true);
        DrawCheckerboard(dl, pos, max, 5.0f);
        if (e.UseMidColor)
        {
            float mid = std::floor(pos.x + width * 0.5f);
            dl->AddRectFilledMultiColor(pos, ImVec2(mid, max.y), ToU32(e.StartColor), ToU32(e.MidColor), ToU32(e.MidColor), ToU32(e.StartColor));
            dl->AddRectFilledMultiColor(ImVec2(mid, pos.y), max, ToU32(e.MidColor), ToU32(e.EndColor), ToU32(e.EndColor), ToU32(e.MidColor));
        }
        else
        {
            dl->AddRectFilledMultiColor(pos, max, ToU32(e.StartColor), ToU32(e.EndColor), ToU32(e.EndColor), ToU32(e.StartColor));
        }
        dl->PopClipRect();
        dl->AddRect(pos, max, U32(Colors::Border), 2.0f);
    }

    bool AcceptPayload(std::string &path, bool (*accept)(const std::string &))
    {
        bool dropped = false;
        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM");
            if (!payload)
                payload = ImGui::AcceptDragDropPayload("TEXTURE_ASSET");
            if (payload)
            {
                std::string candidate(static_cast<const char *>(payload->Data));
                if (accept(candidate))
                {
                    path = candidate;
                    dropped = true;
                }
            }
            ImGui::EndDragDropTarget();
        }
        return dropped;
    }

    bool IsParticleFile(const std::string &path)
    {
        return EditorUI::LowerExtension(path) == SIMPEngine::ParticleSerializer::Extension;
    }

    bool IsImageFile(const std::string &path)
    {
        return EditorUI::IsImageExtension(EditorUI::LowerExtension(path));
    }
}

void InspectorPanel::DrawParticleSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;

    EditorContext &ctx = *m_Context;
    SIMPEngine::Entity entity = m_SelectedEntity;
    SIMPEngine::Scene *const scene = m_Context->Scene;
    const entt::entity handle = entity.GetHandle();

    DrawComponentSection<ParticleEmitterComponent>(ctx, "Particle Emitter", m_SelectedEntity, [this, &ctx, entity, scene, handle](ParticleEmitterComponent &pe)
                                                   {
        auto &e = pe.effect;
        float spacingX = ImGui::GetStyle().ItemSpacing.x;

        PropertyText("Effect", e.Name);

        PropertyLabel("");
        if (Button("Browse Effects...", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)) && m_ParticlePicker)
        {
            m_ParticlePicker->Open("Particle Effects", [this, scene, handle](const SIMPEngine::ParticleEffect &chosen)
                                   {
                if (m_Context->Scene != scene)
                    return;
                SIMPEngine::Entity target(handle, scene);
                if (target)
                    EditorCommands::ApplyParticleEffect(*m_Context, target, chosen); }, e.Name);
        }
        std::string dropped;
        if (AcceptPayload(dropped, IsParticleFile))
        {
            SIMPEngine::ParticleEffect loaded;
            if (SIMPEngine::ParticleSerializer::LoadFile(dropped, loaded))
                EditorCommands::ApplyParticleEffect(ctx, entity, loaded);
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Pick a premade effect, or drop a .particle file here");

        PropertyLabel("");
        float third = std::floor((ImGui::GetContentRegionAvail().x - spacingX * 2.0f) * (1.0f / 3.0f));
        if (Button(pe.playing ? "Pause" : "Play", pe.playing ? ButtonKind::Secondary : ButtonKind::Primary, ImVec2(third, 0)))
        {
            EditorCommands::RecordEdit(ctx);
            pe.playing = !pe.playing;
        }
        ImGui::SameLine();
        if (Button("Restart", ButtonKind::Secondary, ImVec2(third, 0)))
        {
            SIMPEngine::ParticleEmitter::Restart(pe.state);
            pe.playing = true;
        }
        ImGui::SameLine();
        SIMPEngine::Entity self = entity;
        if (Button("Burst", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)) && self.HasComponent<TransformComponent>())
            SIMPEngine::ParticleEmitter::Emit(e, pe.state, SIMPEngine::ParticleSystem::FrameOf(self.GetComponent<TransformComponent>()), std::max(e.Burst, 24));

        char live[64];
        std::snprintf(live, sizeof(live), "%zu / %d", pe.state.Particles.size(), e.MaxParticles);
        PropertyValue("Particles", live, &Colors::TextDim);

        SectionLabel("Emission");
        PropertyBool("Looping", &e.Looping);
        PropertyFloat("Duration", &e.Duration, 0.01f, 0.01f, 600.0f, "%.2f s");
        PropertyFloat("Rate", &e.Rate, 0.5f, 0.0f, 10000.0f, "%.1f / s");
        PropertyInt("Burst", &e.Burst, 0.5f, 0, 10000);
        PropertyInt("Max Particles", &e.MaxParticles, 1.0f, 1, 20000);
        PropertyBool("Prewarm", &e.Prewarm);
        PropertyBool("World Space", &e.WorldSpace);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Off: particles move with the emitter");

        SectionLabel("Emitter Shape");
        static const char *emitters[] = {"Point", "Circle", "Box", "Line"};
        int emitter = static_cast<int>(e.Emitter);
        if (PropertySegmented("Shape", &emitter, emitters, 4))
            e.Emitter = static_cast<SIMPEngine::EmitterShape>(emitter);
        if (e.Emitter == SIMPEngine::EmitterShape::Circle)
            PropertyFloat("Radius", &e.Radius, 0.5f, 0.0f, 10000.0f, "%.1f");
        else if (e.Emitter == SIMPEngine::EmitterShape::Box)
            PropertyVec2("Box Size", &e.BoxSize.x, 0.5f, "%.1f", 0.0f, 100000.0f);
        else if (e.Emitter == SIMPEngine::EmitterShape::Line)
            PropertyFloat("Length", &e.BoxSize.x, 0.5f, 0.0f, 100000.0f, "%.1f");
        if (e.Emitter == SIMPEngine::EmitterShape::Circle || e.Emitter == SIMPEngine::EmitterShape::Box)
            PropertyBool("Edge Only", &e.EmitFromEdge);

        SectionLabel("Motion");
        PropertyFloat("Direction", &e.Direction, 0.5f, -360.0f, 360.0f, "%.0f°");
        PropertyFloat("Spread", &e.Spread, 0.5f, 0.0f, 180.0f, "± %.0f°");
        PropertyRange("Speed", &e.Speed.x, 1.0f, "%.0f", 0.0f, 100000.0f);
        PropertyVec2("Gravity", &e.Gravity.x, 1.0f, "%.0f");
        PropertyFloat("Drag", &e.Drag, 0.02f, 0.0f, 100.0f, "%.2f");
        PropertyFloat("Radial", &e.Radial, 1.0f, -10000.0f, 10000.0f, "%.0f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Acceleration away from the emitter (negative pulls in)");
        PropertyFloat("Orbital", &e.Orbital, 1.0f, -10000.0f, 10000.0f, "%.0f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Acceleration around the emitter");
        PropertyFloat("Turbulence", &e.Turbulence, 0.5f, 0.0f, 10000.0f, "%.0f");
        if (e.Turbulence > 0.0f)
            PropertyFloat("Frequency", &e.TurbulenceFrequency, 0.02f, 0.0f, 50.0f, "%.2f");

        SectionLabel("Lifetime & Size");
        PropertyRange("Lifetime", &e.Lifetime.x, 0.01f, "%.2f s", 0.01f, 600.0f);
        PropertyRange("Size", &e.Size.x, 0.25f, "%.1f", 0.0f, 10000.0f);
        PropertyFloat("End Size", &e.EndSize, 0.01f, 0.0f, 100.0f, "x %.2f");
        PropertyFloat("Stretch", &e.Stretch, 0.001f, 0.0f, 10.0f, "%.3f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Lengthens particles along their velocity (seconds of travel)");
        PropertyRange("Rotation", &e.StartRotation.x, 1.0f, "%.0f°");
        PropertyRange("Spin", &e.AngularVelocity.x, 1.0f, "%.0f°/s");
        PropertyBool("Face Velocity", &e.AlignToVelocity);

        SectionLabel("Color");
        PropertyColorVec4("Start", e.StartColor);
        PropertyBool("Use Middle", &e.UseMidColor);
        if (e.UseMidColor)
            PropertyColorVec4("Middle", e.MidColor);
        PropertyColorVec4("End", e.EndColor);
        DrawGradient(e);
        PropertyFloat("Hue Variation", &e.HueVariation, 0.005f, 0.0f, 1.0f, "%.2f");
        PropertyFloat("Fade In", &e.FadeIn, 0.005f, 0.0f, 1.0f, "%.2f");

        SectionLabel("Render");
        static const char *shapes[] = {"Soft Glow", "Circle", "Square", "Spark", "Star", "Smoke", "Ring", "Texture"};
        int shape = static_cast<int>(e.Shape);
        PropertyLabel("Particle");
        if (ImGui::Combo("##particleshape", &shape, shapes, IM_ARRAYSIZE(shapes)))
        {
            NotifyEdit();
            e.Shape = static_cast<SIMPEngine::ParticleShape>(shape);
        }
        if (e.Shape == SIMPEngine::ParticleShape::Texture)
        {
            std::string current = e.CustomTexture ? e.CustomTexture->GetVFSPath() : std::string();
            PropertyLabel("Texture");
            if (Button(current.empty() ? "Choose image..." : current.c_str(), ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)) && m_AssetPicker)
            {
                m_AssetPicker->Open("Select Particle Texture", "assets://", [this, scene, handle](const std::string &vpath)
                                    {
                    if (m_Context->Scene != scene)
                        return;
                    SIMPEngine::Entity target(handle, scene);
                    if (!target || !target.HasComponent<ParticleEmitterComponent>())
                        return;
                    EditorCommands::RecordEdit(*m_Context);
                    target.GetComponent<ParticleEmitterComponent>().effect.CustomTexture = EditorCommands::LoadTexture(vpath); }, ImageExtensions);
            }
            std::string image;
            if (AcceptPayload(image, IsImageFile))
            {
                EditorCommands::RecordEdit(ctx);
                e.CustomTexture = EditorCommands::LoadTexture(image);
            }
        }
        static const char *blends[] = {"Alpha", "Additive", "Multiply"};
        int blend = static_cast<int>(e.Blend);
        if (PropertySegmented("Blend", &blend, blends, 3))
            e.Blend = static_cast<SIMPEngine::BlendMode>(blend);

        ImGui::Dummy(ImVec2(0, 4));
        PropertyLabel("");
        if (Button("Save as Asset", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        {
            std::string path;
            if (EditorCommands::SaveParticleEffect(e, path))
            {
                s_SavedPath = path;
                s_SavedEntity = handle;
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Writes assets/particles/<Effect>.particle; it then shows up under Saved Effects");
        if (s_SavedEntity == handle && !s_SavedPath.empty())
            PropertyValue("", s_SavedPath.c_str(), &Colors::Success); });
}
