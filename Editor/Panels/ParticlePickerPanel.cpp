#include "ParticlePickerPanel.h"
#include "UI/Widgets.h"

#include <Engine/Particles/ParticleEmitter.h>
#include <Engine/Particles/ParticlePresets.h>
#include <Engine/Particles/ParticleSerializer.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Core/VFS.h>

#include <imgui_internal.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace
{
    constexpr const char *AllCategory = "";
    constexpr const char *ProjectCategory = "Project";

    const char *CategoryIcon(const std::string &category)
    {
        if (category == "Fire & Smoke")
            return ICON_LC_FLAME;
        if (category == "Impact")
            return ICON_LC_ZAP;
        if (category == "Magic")
            return ICON_LC_WAND_SPARKLES;
        if (category == "Weather")
            return ICON_LC_CLOUD_SNOW;
        if (category == "Nature")
            return ICON_LC_LEAF;
        if (category == "Celebration")
            return ICON_LC_PARTY_POPPER;
        if (category == ProjectCategory)
            return ICON_LC_FOLDER;
        return ICON_LC_SPARKLES;
    }

    void Framing(const SIMPEngine::ParticleEffect &e, float &viewHeight, glm::vec2 &anchor)
    {
        float speed = (e.Speed.x + e.Speed.y) * 0.5f;
        float life = (e.Lifetime.x + e.Lifetime.y) * 0.5f;
        float travel = speed * life / (1.0f + e.Drag * life * 0.5f);
        float shape = 0.0f;
        if (e.Emitter == SIMPEngine::EmitterShape::Circle)
            shape = e.Radius * 2.0f;
        else if (e.Emitter == SIMPEngine::EmitterShape::Box)
            shape = std::max(e.BoxSize.x * 0.75f, e.BoxSize.y);
        else if (e.Emitter == SIMPEngine::EmitterShape::Line)
            shape = e.BoxSize.x * 0.75f;
        viewHeight = std::clamp(std::max(travel * 1.4f, shape * 1.3f) + e.Size.y * 2.0f, 120.0f, 600.0f);

        float dir = std::fmod(e.Direction + 360.0f, 360.0f);
        if (e.Spread >= 120.0f || speed < 1.0f)
            anchor = {0.0f, 0.0f};
        else if (dir > 30.0f && dir < 150.0f)
            anchor = {0.0f, -0.35f};
        else if (dir > 210.0f && dir < 330.0f)
            anchor = {0.0f, 0.35f};
        else
            anchor = {0.0f, 0.0f};
    }

    std::string Describe(const SIMPEngine::ParticleEffect &e)
    {
        char text[128];
        if (e.Rate > 0.0f && e.Burst > 0)
            std::snprintf(text, sizeof(text), "%.0f / s  +  burst %d", e.Rate, e.Burst);
        else if (e.Rate > 0.0f)
            std::snprintf(text, sizeof(text), "%.0f / s", e.Rate);
        else
            std::snprintf(text, sizeof(text), "Burst %d every %.1f s", e.Burst, e.Duration);
        return text;
    }
}

void ParticlePickerPanel::Open(const std::string &title, ConfirmCallback onConfirm, const std::string &current)
{
    m_Title = title + "###ParticlePicker";
    m_OnConfirm = std::move(onConfirm);
    m_Search[0] = '\0';
    m_Category = AllCategory;
    Refresh();

    m_Selected = -1;
    for (size_t i = 0; i < m_Entries.size(); ++i)
        if (!current.empty() && m_Entries[i].Name == current)
            m_Selected = static_cast<int>(i);
    if (m_Selected < 0 && !m_Entries.empty())
        m_Selected = 0;
    m_DetailIndex = -1;
    m_Open = true;
}

void ParticlePickerPanel::Refresh()
{
    m_Entries.clear();
    for (const auto &preset : SIMPEngine::ParticlePresets::All())
        m_Entries.push_back({preset.Name, preset.Category, preset.Description, "Built-in", preset.Effect, preset.ViewHeight, preset.Anchor});

    if (auto root = SIMPEngine::VFS::Resolve("assets://"))
    {
        std::error_code ec;
        std::vector<std::filesystem::path> files;
        for (auto it = std::filesystem::recursive_directory_iterator(*root, std::filesystem::directory_options::skip_permission_denied, ec);
             !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
        {
            if (it->is_regular_file(ec) && it->path().extension() == SIMPEngine::ParticleSerializer::Extension)
                files.push_back(it->path());
        }
        std::sort(files.begin(), files.end());

        for (const auto &file : files)
        {
            std::string vpath = "assets://" + std::filesystem::relative(file, *root, ec).generic_string();
            Entry entry;
            if (!SIMPEngine::ParticleSerializer::LoadFile(vpath, entry.Effect))
                continue;
            entry.Name = entry.Effect.Name.empty() ? file.stem().string() : entry.Effect.Name;
            entry.Category = ProjectCategory;
            entry.Description = "Saved effect";
            entry.Source = vpath;
            Framing(entry.Effect, entry.ViewHeight, entry.Anchor);
            m_Entries.push_back(std::move(entry));
        }
    }

    m_Previews.clear();
    m_Previews.resize(m_Entries.size());
}

bool ParticlePickerPanel::Visible(const Entry &entry) const
{
    if (!m_Category.empty() && entry.Category != m_Category)
        return false;
    return EditorUI::ContainsInsensitive(entry.Name.c_str(), m_Search) || EditorUI::ContainsInsensitive(entry.Category.c_str(), m_Search);
}

ImTextureID ParticlePickerPanel::RenderPreview(const Entry &entry, Preview &preview, int width, int height)
{
    using namespace SIMPEngine;

    width = std::max(8, width);
    height = std::max(8, height);
    if (!preview.Target)
        preview.Target = std::make_unique<Framebuffer>(width, height);
    else if (preview.Target->GetWidth() != width || preview.Target->GetHeight() != height)
        preview.Target->Resize(width, height);

    int frameIndex = ImGui::GetFrameCount();
    if (preview.LastFrame == frameIndex)
        return EditorUI::TextureId(preview.Target->GetColorAttachment());
    preview.LastFrame = frameIndex;

    const ParticleEffect &effect = entry.Effect;
    EmitterFrame frame;
    float dt = std::min(ImGui::GetIO().DeltaTime, 0.05f);

    if (!preview.Warmed)
    {
        ParticleEmitter::Restart(preview.State);
        float warm = effect.Rate > 0.0f ? std::min(effect.Lifetime.y, 3.0f) : 0.15f;
        ParticleEmitter::Prewarm(effect, preview.State, frame, warm);
        preview.Warmed = true;
        preview.Idle = 0.0f;
    }
    else
    {
        ParticleEmitter::Simulate(effect, preview.State, frame, dt);
        if (!ParticleEmitter::IsAlive(effect, preview.State))
        {
            preview.Idle += dt;
            if (preview.Idle > 0.5f)
            {
                ParticleEmitter::Restart(preview.State);
                preview.Idle = 0.0f;
            }
        }
    }

    float viewH = entry.ViewHeight * 0.78f;
    float viewW = viewH * static_cast<float>(width) / static_cast<float>(height);
    glm::vec2 center = -entry.Anchor * glm::vec2(viewW, viewH);
    glm::mat4 proj = glm::ortho(center.x - viewW * 0.5f, center.x + viewW * 0.5f, center.y - viewH * 0.5f, center.y + viewH * 0.5f, -100000.0f, 100000.0f);

    const ImVec4 &bg = EditorUI::Colors::Canvas;
    Renderer::BeginRenderTarget(*preview.Target);
    Renderer::SetClearColor(bg.x * 0.7f, bg.y * 0.7f, bg.z * 0.75f, 1.0f);
    Renderer::Clear();
    Renderer::BeginScene(glm::mat4(1.0f), proj);
    ParticleEmitter::Render(effect, preview.State, frame, 0.0f);
    Renderer::EndScene();
    Renderer::EndRenderTarget();

    return EditorUI::TextureId(preview.Target->GetColorAttachment());
}

void ParticlePickerPanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (!m_Open)
        return;

    if (!BeginModal(m_Title.c_str(), &m_Open, ImVec2(1280, 800)))
    {
        if (!m_Open)
            Close();
        return;
    }

    int total = 0;
    for (const auto &entry : m_Entries)
        total += Visible(entry) ? 1 : 0;

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%d effect%s", total, total == 1 ? "" : "s");
    ImGui::PopStyleColor();
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 260.0f);
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    SearchBox("##particlesearch", m_Search, sizeof(m_Search), "Search effects", -FLT_MIN);

    float footerH = ImGui::GetFrameHeight() + 14.0f;
    float bodyH = ImGui::GetContentRegionAvail().y - footerH;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Header);
    DrawSidebar(bodyH);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    DrawGrid(bodyH);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    DrawDetails(bodyH);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::Dummy(ImVec2(0, 4));

    bool valid = m_Selected >= 0 && m_Selected < static_cast<int>(m_Entries.size());
    float bw = 120.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Double-click or press Enter to use an effect. Every setting stays editable afterwards.");
    ImGui::PopStyleColor();

    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - bw * 2.0f - spacing);
    if (Button("Cancel", ButtonKind::Secondary, ImVec2(bw, 0)))
    {
        Close();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!valid);
    bool enter = valid && ImGui::IsKeyPressed(ImGuiKey_Enter, false) && !ImGui::IsAnyItemActive();
    if (Button("Use Effect", ButtonKind::Primary, ImVec2(bw, 0)) || enter)
    {
        Confirm();
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();

    if (!m_Open)
        ImGui::CloseCurrentPopup();

    EndModal();
}

void ParticlePickerPanel::DrawSidebar(float height)
{
    using namespace EditorUI;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 6));
    ImGui::BeginChild("##categories", ImVec2(200, height), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

    auto row = [&](const std::string &category, const char *label)
    {
        int count = 0;
        for (const auto &entry : m_Entries)
            if (category.empty() || entry.Category == category)
                ++count;

        char trailing[16];
        std::snprintf(trailing, sizeof(trailing), "%d", count);

        TreeRowOptions opt;
        opt.Leaf = true;
        opt.Selected = m_Category == category;
        opt.Trailing = trailing;
        std::string id = "cat_" + category;
        if (TreeRow(id.c_str(), category.empty() ? ICON_LC_LAYOUT_GRID : CategoryIcon(category), category.empty() ? Colors::Accent : Colors::NodeTint, label, opt).Clicked)
            m_Category = category;
    };

    row(AllCategory, "All Effects");
    ImGui::Dummy(ImVec2(0, 4));
    SectionLabel("Built-in");
    for (const auto &category : SIMPEngine::ParticlePresets::Categories())
        row(category, category.c_str());
    ImGui::Dummy(ImVec2(0, 4));
    SectionLabel("Project");
    row(ProjectCategory, "Saved");

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void ParticlePickerPanel::DrawGrid(float height)
{
    using namespace EditorUI;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::BeginChild("##effects", ImVec2(ImGui::GetContentRegionAvail().x - 300.0f, height), ImGuiChildFlags_AlwaysUseWindowPadding);

    std::vector<int> visible;
    for (size_t i = 0; i < m_Entries.size(); ++i)
        if (Visible(m_Entries[i]))
            visible.push_back(static_cast<int>(i));

    if (!visible.empty() && std::find(visible.begin(), visible.end(), m_Selected) == visible.end())
        m_Selected = visible.front();

    if (visible.empty())
    {
        if (m_Category == ProjectCategory && !m_Search[0])
            EmptyState("No saved effects", "Use \"Save as Asset\" in the Particle Emitter inspector to add one to assets/particles.");
        else
            EmptyState("No effects found", "Try another search or category.");
        ImGui::EndChild();
        ImGui::PopStyleVar();
        return;
    }

    float spacing = 10.0f;
    float minW = 205.0f;
    float avail = ImGui::GetContentRegionAvail().x;
    int columns = std::max(1, static_cast<int>((avail + spacing) / (minW + spacing)));
    float tileW = std::floor((avail - spacing * static_cast<float>(columns - 1)) / static_cast<float>(columns));
    float pad = 5.0f;
    float thumbW = tileW - pad * 2.0f;
    float thumbH = std::floor(thumbW * 0.72f);
    float fontSize = ImGui::GetFontSize();
    float tileH = pad + thumbH + 6.0f + fontSize + 2.0f + fontSize + pad;
    float scale = ImGui::GetIO().DisplayFramebufferScale.x;

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    ImDrawList *dl = window->DrawList;
    bool confirm = false;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
    for (size_t n = 0; n < visible.size(); ++n)
    {
        int index = visible[n];
        const Entry &entry = m_Entries[static_cast<size_t>(index)];
        if (n % static_cast<size_t>(columns))
            ImGui::SameLine();

        ImGui::PushID(index);
        ImVec2 pos = window->DC.CursorPos;
        ImRect bb(pos, ImVec2(pos.x + tileW, pos.y + tileH));
        ImGuiID id = window->GetID("tile");
        ImGui::ItemSize(bb, 0.0f);
        if (ImGui::ItemAdd(bb, id))
        {
            bool hovered = false, held = false;
            if (ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick))
                m_Selected = index;
            if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                m_Selected = index;
                confirm = true;
            }

            bool selected = m_Selected == index;
            if (selected)
                dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Selection), Metrics::FrameRounding);
            else if (hovered)
                dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Hover), Metrics::FrameRounding);

            ImVec2 tMin(bb.Min.x + pad, bb.Min.y + pad);
            ImVec2 tMax(tMin.x + thumbW, tMin.y + thumbH);
            ImTextureID texture = RenderPreview(entry, m_Previews[static_cast<size_t>(index)], static_cast<int>(thumbW * scale), static_cast<int>(thumbH * scale));
            dl->AddImageRounded(texture, tMin, tMax, ImVec2(0, 1), ImVec2(1, 0), IM_COL32_WHITE, Metrics::FrameRounding);
            dl->AddRect(tMin, tMax, U32(selected ? Colors::Accent : Colors::Border), Metrics::FrameRounding, 0, selected ? 2.0f : 1.0f);

            float textY = tMax.y + 6.0f;
            ImGui::PushFont(GetFont(FontRole::Medium), 0.0f);
            dl->PushClipRect(ImVec2(bb.Min.x + pad, textY), ImVec2(bb.Max.x - pad, bb.Max.y), true);
            dl->AddText(ImVec2(bb.Min.x + pad + 1.0f, textY), U32(Colors::Text), entry.Name.c_str());
            ImGui::PopFont();
            dl->AddText(ImVec2(bb.Min.x + pad + 1.0f, textY + fontSize + 2.0f), U32(Colors::TextFaint), entry.Category.c_str());
            dl->PopClipRect();

            if (hovered && !entry.Description.empty())
                ImGui::SetItemTooltip("%s", entry.Description.c_str());
        }
        ImGui::PopID();
    }
    ImGui::PopStyleVar();

    ImGui::EndChild();
    ImGui::PopStyleVar();

    if (confirm)
    {
        Confirm();
        ImGui::CloseCurrentPopup();
    }
}

void ParticlePickerPanel::DrawDetails(float height)
{
    using namespace EditorUI;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::BeginChild("##details", ImVec2(0, height), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

    if (m_Selected < 0 || m_Selected >= static_cast<int>(m_Entries.size()))
    {
        EmptyState("No effect selected");
        ImGui::EndChild();
        ImGui::PopStyleVar();
        return;
    }

    const Entry &entry = m_Entries[static_cast<size_t>(m_Selected)];
    if (m_DetailIndex != m_Selected)
    {
        m_DetailPreview.Warmed = false;
        m_DetailIndex = m_Selected;
    }

    float w = ImGui::GetContentRegionAvail().x;
    float h = std::floor(w * 0.9f);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float scale = ImGui::GetIO().DisplayFramebufferScale.x;
    ImTextureID texture = RenderPreview(entry, m_DetailPreview, static_cast<int>(w * scale), static_cast<int>(h * scale));
    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddImageRounded(texture, pos, ImVec2(pos.x + w, pos.y + h), ImVec2(0, 1), ImVec2(1, 0), IM_COL32_WHITE, Metrics::FrameRounding);
    dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), U32(Colors::Border), Metrics::FrameRounding);
    ImGui::Dummy(ImVec2(w, h));

    char particles[48];
    std::snprintf(particles, sizeof(particles), "%zu live", m_DetailPreview.State.Particles.size());
    ImVec2 ts = ImGui::CalcTextSize(particles);
    dl->AddText(ImVec2(pos.x + w - ts.x - 8.0f, pos.y + h - ts.y - 6.0f), U32(Colors::TextFaint), particles);

    ImGui::Dummy(ImVec2(0, 4));
    ImGui::PushFont(GetFont(FontRole::Medium), 0.0f);
    ImGui::TextUnformatted(entry.Name.c_str());
    ImGui::PopFont();
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::TextWrapped("%s  ·  %s", entry.Category.c_str(), entry.Source.c_str());
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::TextWrapped("%s", entry.Description.c_str());
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6));

    const auto &e = entry.Effect;
    char text[96];
    PropertyValue("Emission", Describe(e).c_str());
    std::snprintf(text, sizeof(text), "%.1f - %.1f s", e.Lifetime.x, e.Lifetime.y);
    PropertyValue("Lifetime", text);
    std::snprintf(text, sizeof(text), "%s  ·  %s", SIMPEngine::ParticleSerializer::ShapeName(e.Shape),
                  e.Blend == SIMPEngine::BlendMode::Additive ? "additive" : (e.Blend == SIMPEngine::BlendMode::Multiply ? "multiply" : "alpha"));
    PropertyValue("Look", text);
    PropertyValue("Emitter", SIMPEngine::ParticleSerializer::EmitterName(e.Emitter));
    std::snprintf(text, sizeof(text), "%d", e.MaxParticles);
    PropertyValue("Max", text);
    PropertyValue("Loop", e.Looping ? "yes" : "once", e.Looping ? nullptr : &Colors::Warning);

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void ParticlePickerPanel::Confirm()
{
    if (m_OnConfirm && m_Selected >= 0 && m_Selected < static_cast<int>(m_Entries.size()))
        m_OnConfirm(m_Entries[static_cast<size_t>(m_Selected)].Effect);
    Close();
}

void ParticlePickerPanel::Close()
{
    m_Open = false;
    m_OnConfirm = nullptr;
    m_Previews.clear();
    m_DetailPreview = Preview{};
    m_DetailIndex = -1;
}
