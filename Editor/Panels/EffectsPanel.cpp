#include "EffectsPanel.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <Engine/Animation/Tween.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    using namespace SIMPEngine;

    constexpr float Pi = 3.14159265f;
    constexpr float CardWidth = 150.0f;
    constexpr float PreviewHeight = 96.0f;
    constexpr float CardHeight = 126.0f;
    constexpr float CardGap = 8.0f;
    constexpr float DetailWidth = 380.0f;
    constexpr float HoldSeconds = 0.7f;
    constexpr const char *CategoryItems[] = {"All", "Entrance", "Exit", "Attention", "Loop"};

    ImU32 Col(const ImVec4 &c, float alpha = 1.0f)
    {
        return ImGui::GetColorU32(ImVec4(c.x, c.y, c.z, c.w * alpha));
    }

    ImVec4 CategoryColor(const std::string &category)
    {
        if (category == "Entrance")
            return EditorUI::Colors::Success;
        if (category == "Exit")
            return EditorUI::Colors::Warning;
        if (category == "Loop")
            return EditorUI::Colors::NodeTint;
        return EditorUI::Colors::Accent;
    }

    void ResetSubject(Entity e)
    {
        if (!e)
            return;
        auto &t = e.GetComponent<TransformComponent>();
        t.position = {0.0f, 0.0f};
        t.scale = {1.0f, 1.0f};
        t.rotation = 0.0f;
        e.GetComponent<RenderComponent>().color.a = 255;
    }

    void DrawSubject(ImDrawList *dl, Entity e, ImVec2 center, float unit, float size, const ImVec4 &color, bool ghost)
    {
        const auto &t = e.GetComponent<TransformComponent>();
        const float alpha = e.GetComponent<RenderComponent>().color.a / 255.0f;
        const float half = size * 0.5f;
        const float radius = size * 0.22f;

        if (ghost)
            dl->AddRect(ImVec2(center.x - half, center.y - half), ImVec2(center.x + half, center.y + half), Col(EditorUI::Colors::TextFaint, 0.45f), radius, 0, 1.0f);
        if (alpha <= 0.003f)
            return;

        const float rad = t.rotation * (Pi / 180.0f);
        const float c = std::cos(rad), s = std::sin(rad);
        const float ox = center.x + t.position.x * unit;
        const float oy = center.y - t.position.y * unit;
        auto place = [&](float lx, float ly)
        {
            const float wx = lx * t.scale.x, wy = ly * t.scale.y;
            return ImVec2(ox + wx * c - wy * s, oy - (wx * s + wy * c));
        };

        std::vector<ImVec2> points;
        points.reserve(28);
        const float centers[4][3] = {{half - radius, half - radius, 0.0f}, {-half + radius, half - radius, Pi * 0.5f}, {-half + radius, -half + radius, Pi}, {half - radius, -half + radius, Pi * 1.5f}};
        for (const auto &corner : centers)
            for (int i = 0; i <= 5; ++i)
            {
                const float a = corner[2] + static_cast<float>(i) * (Pi * 0.5f / 5.0f);
                points.push_back(place(corner[0] + radius * std::cos(a), corner[1] + radius * std::sin(a)));
            }

        float area = 0.0f;
        for (size_t i = 0; i < points.size(); ++i)
        {
            const ImVec2 &a = points[i];
            const ImVec2 &b = points[(i + 1) % points.size()];
            area += a.x * b.y - b.x * a.y;
        }
        if (area < 0.0f)
            std::reverse(points.begin(), points.end());

        dl->AddConvexPolyFilled(points.data(), static_cast<int>(points.size()), Col(color, alpha));
        dl->AddCircleFilled(place(half * 0.42f, half * 0.42f), std::max(2.0f, size * 0.09f), Col(ImVec4(1, 1, 1, 1), alpha * 0.9f));
    }

    float EaseClock(float clock, float offset)
    {
        const float local = std::fmod(clock + offset, 2.2f);
        return std::clamp(local / 1.4f, 0.0f, 1.0f);
    }

    void DrawCurveGraph(ImDrawList *dl, ImVec2 min, ImVec2 max, const Curve &curve, float t, bool big)
    {
        const float pad = big ? 26.0f : 12.0f;
        const float trackHeight = big ? 34.0f : 16.0f;
        const ImVec2 a(min.x + pad, min.y + pad * 0.6f);
        const ImVec2 b(max.x - pad, max.y - trackHeight - pad * 0.4f);
        constexpr float yMin = -0.4f, yMax = 1.4f;
        auto map = [&](float x, float v)
        {
            return ImVec2(a.x + x * (b.x - a.x), b.y - (v - yMin) / (yMax - yMin) * (b.y - a.y));
        };

        dl->PushClipRect(min, max, true);
        for (float guide : {0.0f, 1.0f})
            dl->AddLine(map(0.0f, guide), map(1.0f, guide), Col(EditorUI::Colors::TextFaint, 0.5f), 1.0f);

        std::vector<ImVec2> points;
        points.reserve(65);
        for (int i = 0; i <= 64; ++i)
        {
            const float x = static_cast<float>(i) / 64.0f;
            points.push_back(map(x, curve.Evaluate(x)));
        }
        dl->AddPolyline(points.data(), static_cast<int>(points.size()), Col(EditorUI::Colors::TextDim, 0.9f), 0, big ? 2.5f : 1.8f);

        const int shown = std::clamp(static_cast<int>(t * 64.0f), 1, 64);
        dl->AddPolyline(points.data(), shown + 1, Col(EditorUI::Colors::Accent), 0, big ? 3.0f : 2.2f);
        const float value = curve.Evaluate(t);
        dl->AddCircleFilled(map(t, value), big ? 6.0f : 4.0f, Col(EditorUI::Colors::Accent));

        const float trackY = max.y - trackHeight * 0.5f - 2.0f;
        const ImVec2 t0(a.x, trackY), t1(b.x, trackY);
        dl->AddLine(t0, t1, Col(EditorUI::Colors::TextFaint, 0.6f), 2.0f);
        dl->AddCircleFilled(ImVec2(t0.x + value * (t1.x - t0.x), trackY), big ? 8.0f : 5.0f, Col(EditorUI::Colors::Success));
        dl->PopClipRect();
    }

    std::string ShortEaseName(const std::string &name)
    {
        if (name.size() > 4 && name.rfind("ease", 0) == 0)
        {
            std::string s = name.substr(4);
            s[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[0])));
            return s;
        }
        return name;
    }

    int CardColumns(float width)
    {
        return std::max(1, static_cast<int>((width + CardGap) / (CardWidth + CardGap)));
    }
}

EffectsPanel::EffectsPanel()
{
    for (const std::string &name : Curve::Names())
    {
        CurveEntry entry;
        entry.Name = name;
        if (Curve::FromName(name, entry.Curve))
            m_Curves.push_back(std::move(entry));
    }
    for (size_t i = 0; i < m_Curves.size(); ++i)
        if (m_Curves[i].Name == "easeOutBack")
            m_SelectedCurve = static_cast<int>(i);
}

EffectsPanel::~EffectsPanel() = default;

void EffectsPanel::EnsureScene()
{
    if (m_Scene)
        return;
    m_Scene = std::make_unique<Scene>("Effects");
    const auto &all = EffectPresets::All();
    m_Previews.assign(all.size(), Preview{});
    for (size_t i = 0; i < all.size(); ++i)
    {
        m_Previews[i].Subject = m_Scene->CreateEntity(all[i].Name);
        m_Previews[i].Subject.AddComponent<RenderComponent>();
        m_Previews[i].Wait = HoldSeconds - 0.04f * static_cast<float>(i % 12);
    }
    m_Detail.Subject = m_Scene->CreateEntity("Detail");
    m_Detail.Subject.AddComponent<RenderComponent>();
    SelectEffect(0);
}

void EffectsPanel::Restart(Preview &preview, const EffectPreset &preset, const EffectOptions &options)
{
    TweenSystem &tweens = m_Scene->GetTweens();
    tweens.KillTarget(preview.Subject.GetHandle());
    ResetSubject(preview.Subject);
    preview.Tween = EffectPresets::Play(tweens, preview.Subject.GetHandle(), preset, options);
    preview.Wait = 0.0f;
}

void EffectsPanel::Tick(float dt)
{
    TweenSystem &tweens = m_Scene->GetTweens();
    tweens.Update(dt);
    m_Clock += dt;
    if (m_ToastTime > 0.0f)
        m_ToastTime -= dt;

    const auto &all = EffectPresets::All();
    for (size_t i = 0; i < m_Previews.size(); ++i)
    {
        Preview &preview = m_Previews[i];
        if (preview.Tween != 0 && tweens.IsRunning(preview.Tween))
            continue;
        preview.Wait += dt;
        if (preview.Wait >= HoldSeconds)
            Restart(preview, all[i], EffectOptions{});
    }

    const EffectPreset &selected = all[static_cast<size_t>(m_Selected)];
    if (m_Restart)
    {
        m_Restart = false;
        Restart(m_Detail, selected, m_Options);
    }
    else if (m_Detail.Tween == 0 || !tweens.IsRunning(m_Detail.Tween))
    {
        m_Detail.Wait += dt;
        if (m_Detail.Wait >= HoldSeconds + 0.3f)
            Restart(m_Detail, selected, m_Options);
    }
}

void EffectsPanel::SelectEffect(int index)
{
    const auto &all = EffectPresets::All();
    m_Selected = std::clamp(index, 0, static_cast<int>(all.size()) - 1);
    const EffectPreset &preset = all[static_cast<size_t>(m_Selected)];
    m_Options = EffectOptions{};
    m_Duration = preset.Duration;
    m_LoopMode = 0;
    RefreshCode();
    m_Restart = true;
}

void EffectsPanel::RefreshCode()
{
    const EffectPreset &preset = EffectPresets::All()[static_cast<size_t>(m_Selected)];
    m_Options.Duration = std::fabs(m_Duration - preset.Duration) > 0.0005f ? m_Duration : 0.0f;
    m_Options.Loop = m_LoopMode == 0 ? -2 : (m_LoopMode == 1 ? 0 : -1);
    m_OneLiner = EffectPresets::OneLiner(preset);
    m_PlayCall = EffectPresets::PlayCall(preset, m_Options);
    m_Expanded = EffectPresets::ExpandedLua(preset, m_Options);
}

void EffectsPanel::Copy(const std::string &text, const char *what)
{
    ImGui::SetClipboardText(text.c_str());
    m_Toast = std::string("Copied ") + what;
    m_ToastTime = 1.6f;
}

void EffectsPanel::CodeBlock(const char *title, const char *id, std::string &code, float height)
{
    EditorUI::SectionLabel(title);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 52.0f + ImGui::GetCursorPosX());
    ImGui::PushID(id);
    if (EditorUI::Button("Copy"))
        Copy(code, "code");
    ImGui::PopID();
    ImGui::InputTextMultiline((std::string("##") + id).c_str(), &code, ImVec2(-FLT_MIN, height), ImGuiInputTextFlags_ReadOnly);
}

bool EffectsPanel::DrawEffectCard(int index, bool selected)
{
    const EffectPreset &preset = EffectPresets::All()[static_cast<size_t>(index)];
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 end(pos.x + CardWidth, pos.y + CardHeight);

    ImGui::PushID(index);
    const bool clicked = ImGui::InvisibleButton("card", ImVec2(CardWidth, CardHeight));
    const bool hovered = ImGui::IsItemHovered();
    const bool doubleClicked = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    ImGui::PopID();

    dl->AddRectFilled(pos, end, Col(hovered ? EditorUI::Colors::FrameHover : EditorUI::Colors::Frame), 7.0f);
    const ImVec2 areaMin(pos.x + 3.0f, pos.y + 3.0f), areaMax(end.x - 3.0f, pos.y + PreviewHeight);
    dl->AddRectFilled(areaMin, areaMax, Col(EditorUI::Colors::Canvas), 5.0f);

    dl->PushClipRect(areaMin, areaMax, true);
    DrawSubject(dl, m_Previews[static_cast<size_t>(index)].Subject, ImVec2((areaMin.x + areaMax.x) * 0.5f, (areaMin.y + areaMax.y) * 0.5f), 0.34f, 36.0f, CategoryColor(preset.Category), true);
    dl->PopClipRect();

    const ImVec2 textPos(pos.x + 9.0f, pos.y + PreviewHeight + 7.0f);
    dl->PushClipRect(ImVec2(pos.x + 4.0f, textPos.y), ImVec2(end.x - 14.0f, end.y), true);
    dl->AddText(textPos, Col(EditorUI::Colors::Text), preset.Label.c_str());
    dl->PopClipRect();
    dl->AddCircleFilled(ImVec2(end.x - 11.0f, textPos.y + 7.0f), 3.5f, Col(CategoryColor(preset.Category)));
    if (selected)
        dl->AddRect(pos, end, Col(EditorUI::Colors::Accent), 7.0f, 0, 2.0f);

    if (hovered)
        ImGui::SetTooltip("%s\n%s\nDouble-click to copy the one-liner", preset.Description.c_str(), EffectPresets::OneLiner(preset).c_str());
    if (doubleClicked)
        Copy(EffectPresets::OneLiner(preset), "one-liner");
    return clicked;
}

void EffectsPanel::DrawEffectGrid(float width)
{
    const auto &all = EffectPresets::All();
    const int columns = CardColumns(width);
    int shown = 0;
    for (size_t i = 0; i < all.size(); ++i)
    {
        const EffectPreset &preset = all[i];
        if (m_Category > 0 && preset.Category != CategoryItems[m_Category])
            continue;
        if (m_Search[0] && !EditorUI::ContainsInsensitive(preset.Label.c_str(), m_Search) && !EditorUI::ContainsInsensitive(preset.Name.c_str(), m_Search) &&
            !EditorUI::ContainsInsensitive(preset.Description.c_str(), m_Search))
            continue;
        if (shown % columns != 0)
            ImGui::SameLine(0.0f, CardGap);
        if (DrawEffectCard(static_cast<int>(i), static_cast<int>(i) == m_Selected))
            SelectEffect(static_cast<int>(i));
        ++shown;
    }
    if (shown == 0)
        EditorUI::EmptyState("No effects match", "Try another search or category.");
}

void EffectsPanel::DrawEffectDetail()
{
    const EffectPreset &preset = EffectPresets::All()[static_cast<size_t>(m_Selected)];
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const float width = ImGui::GetContentRegionAvail().x;
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 end(pos.x + width, pos.y + 210.0f);
    dl->AddRectFilled(pos, end, Col(EditorUI::Colors::Canvas), 8.0f);
    dl->PushClipRect(pos, end, true);
    DrawSubject(dl, m_Detail.Subject, ImVec2((pos.x + end.x) * 0.5f, (pos.y + end.y) * 0.5f), 0.9f, 64.0f, CategoryColor(preset.Category), true);
    dl->PopClipRect();
    ImGui::Dummy(ImVec2(width, 210.0f));

    ImGui::Spacing();
    ImGui::TextUnformatted(preset.Label.c_str());
    ImGui::SameLine();
    ImGui::TextColored(CategoryColor(preset.Category), "%s", preset.Category.c_str());
    ImGui::PushStyleColor(ImGuiCol_Text, EditorUI::Colors::TextDim);
    ImGui::TextWrapped("%s", preset.Description.c_str());
    ImGui::PopStyleColor();
    ImGui::Spacing();

    bool changed = false;
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::SliderFloat("##duration", &m_Duration, 0.1f, 3.0f, "Duration  %.2f s");
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::SliderFloat("##distance", &m_Options.Distance, 0.25f, 3.0f, "Distance  x%.2f");
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::SliderFloat("##amount", &m_Options.Amount, 0.25f, 3.0f, "Amount  x%.2f");
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::SliderFloat("##delay", &m_Options.Delay, 0.0f, 2.0f, "Delay  %.2f s");
    const char *loops[] = {"Loop: effect default", "Loop: play once", "Loop: forever"};
    ImGui::SetNextItemWidth(-FLT_MIN);
    changed |= ImGui::Combo("##loop", &m_LoopMode, loops, 3);
    if (changed)
    {
        RefreshCode();
        m_Restart = true;
    }

    if (EditorUI::Button("Replay"))
        m_Restart = true;
    ImGui::SameLine();
    if (EditorUI::Button("Reset options"))
        SelectEffect(m_Selected);
    if (m_ToastTime > 0.0f)
    {
        ImGui::SameLine();
        ImGui::TextColored(EditorUI::Colors::Success, "%s", m_Toast.c_str());
    }

    ImGui::Spacing();
    CodeBlock("One-liner", "oneliner", m_OneLiner, 30.0f);
    CodeBlock("With options", "playcall", m_PlayCall, 30.0f);
    CodeBlock("Expanded Tween code", "expanded", m_Expanded, std::max(90.0f, ImGui::GetContentRegionAvail().y - 4.0f));
}

void EffectsPanel::DrawEffects()
{
    ImGui::SetNextItemWidth(220.0f);
    EditorUI::SearchBox("##effectSearch", m_Search, sizeof(m_Search), "Search effects", 220.0f);
    ImGui::SameLine();
    EditorUI::SegmentedControl("##category", &m_Category, CategoryItems, 5, 360.0f);
    ImGui::Spacing();

    const float gridWidth = std::max(CardWidth, ImGui::GetContentRegionAvail().x - DetailWidth - ImGui::GetStyle().ItemSpacing.x);
    ImGui::BeginChild("##effectGrid", ImVec2(gridWidth, 0.0f), ImGuiChildFlags_None);
    DrawEffectGrid(ImGui::GetContentRegionAvail().x);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##effectDetail", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
    DrawEffectDetail();
    ImGui::EndChild();
}

bool EffectsPanel::DrawCurveCard(int index, bool selected)
{
    const CurveEntry &entry = m_Curves[static_cast<size_t>(index)];
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 end(pos.x + CardWidth, pos.y + CardHeight);

    ImGui::PushID(index);
    const bool clicked = ImGui::InvisibleButton("card", ImVec2(CardWidth, CardHeight));
    const bool hovered = ImGui::IsItemHovered();
    const bool doubleClicked = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    ImGui::PopID();

    dl->AddRectFilled(pos, end, Col(hovered ? EditorUI::Colors::FrameHover : EditorUI::Colors::Frame), 7.0f);
    const ImVec2 areaMin(pos.x + 3.0f, pos.y + 3.0f), areaMax(end.x - 3.0f, pos.y + PreviewHeight);
    dl->AddRectFilled(areaMin, areaMax, Col(EditorUI::Colors::Canvas), 5.0f);
    DrawCurveGraph(dl, areaMin, areaMax, entry.Curve, EaseClock(m_Clock, static_cast<float>(index % 7) * 0.05f), false);

    const ImVec2 textPos(pos.x + 9.0f, pos.y + PreviewHeight + 7.0f);
    dl->PushClipRect(ImVec2(pos.x + 4.0f, textPos.y), ImVec2(end.x - 4.0f, end.y), true);
    dl->AddText(textPos, Col(EditorUI::Colors::Text), entry.Name.c_str());
    dl->PopClipRect();
    if (selected)
        dl->AddRect(pos, end, Col(EditorUI::Colors::Accent), 7.0f, 0, 2.0f);

    if (hovered)
        ImGui::SetTooltip("\"%s\"\nDouble-click to copy the name", entry.Name.c_str());
    if (doubleClicked)
        Copy("\"" + entry.Name + "\"", "curve name");
    return clicked;
}

void EffectsPanel::DrawEasingGrid(float width)
{
    const int columns = CardColumns(width);
    int shown = 0;
    for (size_t i = 0; i < m_Curves.size(); ++i)
    {
        if (m_CurveSearch[0] && !EditorUI::ContainsInsensitive(m_Curves[i].Name.c_str(), m_CurveSearch))
            continue;
        if (shown % columns != 0)
            ImGui::SameLine(0.0f, CardGap);
        if (DrawCurveCard(static_cast<int>(i), static_cast<int>(i) == m_SelectedCurve))
            m_SelectedCurve = static_cast<int>(i);
        ++shown;
    }
    if (shown == 0)
        EditorUI::EmptyState("No curves match", "Try another search.");
}

void EffectsPanel::DrawEasingDetail()
{
    const CurveEntry &entry = m_Curves[static_cast<size_t>(m_SelectedCurve)];
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const float width = ImGui::GetContentRegionAvail().x;
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 end(pos.x + width, pos.y + 250.0f);
    dl->AddRectFilled(pos, end, Col(EditorUI::Colors::Canvas), 8.0f);
    const float t = EaseClock(m_Clock, 0.0f);
    DrawCurveGraph(dl, pos, end, entry.Curve, t, true);
    ImGui::Dummy(ImVec2(width, 250.0f));

    ImGui::Spacing();
    ImGui::TextUnformatted(entry.Name.c_str());
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, EditorUI::Colors::TextDim);
    ImGui::Text("t = %.2f   value = %.3f", t, entry.Curve.Evaluate(t));
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::SliderFloat("##curveSeconds", &m_CurveSeconds, 0.1f, 3.0f, "Duration in the code  %.2f s");
    if (m_ToastTime > 0.0f)
        ImGui::TextColored(EditorUI::Colors::Success, "%s", m_Toast.c_str());
    ImGui::Spacing();

    char seconds[16];
    std::snprintf(seconds, sizeof(seconds), "%.2f", m_CurveSeconds);
    std::string name = "\"" + entry.Name + "\"";
    std::string tween = "Tween.To(self.entity, {x = 300}, " + std::string(seconds) + ", " + name + ")";
    std::string ease = "Ease." + ShortEaseName(entry.Name) + "(t)";
    std::string evaluate = "Curves.Evaluate(" + name + ", t)";
    CodeBlock("Curve name", "curvename", name, 30.0f);
    CodeBlock("Animate with it", "curvetween", tween, 30.0f);
    CodeBlock("Use the value directly", "curveease", ease, 30.0f);
    CodeBlock("Or evaluate by name", "curveeval", evaluate, 30.0f);
}

void EffectsPanel::DrawEasing()
{
    EditorUI::SearchBox("##curveSearch", m_CurveSearch, sizeof(m_CurveSearch), "Search easings", 260.0f);
    ImGui::Spacing();
    const float gridWidth = std::max(CardWidth, ImGui::GetContentRegionAvail().x - DetailWidth - ImGui::GetStyle().ItemSpacing.x);
    ImGui::BeginChild("##curveGrid", ImVec2(gridWidth, 0.0f), ImGuiChildFlags_None);
    DrawEasingGrid(ImGui::GetContentRegionAvail().x);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##curveDetail", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
    DrawEasingDetail();
    ImGui::EndChild();
}

void EffectsPanel::OnImGuiRender()
{
    if (!m_Visible)
        return;

    if (ImGuiWindow *viewport = ImGui::FindWindowByName("Viewport###Viewport"); viewport && viewport->DockId)
        ImGui::SetNextWindowDockID(viewport->DockId, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Effects###Effects", &m_Visible))
    {
        ImGui::End();
        return;
    }

    EnsureScene();
    Tick(std::min(ImGui::GetIO().DeltaTime, 0.05f));

    if (ImGui::BeginTabBar("##effectTabs"))
    {
        if (ImGui::BeginTabItem("Effects"))
        {
            DrawEffects();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Easing"))
        {
            DrawEasing();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}
