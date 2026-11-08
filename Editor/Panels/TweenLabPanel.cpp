#include "TweenLabPanel.h"
#include "UI/Widgets.h"

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <Engine/Animation/Curves.h>
#include <Engine/Animation/Tween.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Vector/VectorShape.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace
{
    const char *ShapeNames[] = {"Circle", "Square", "Rounded", "Star", "Heart", "Hexagon", "Arrow", "Line", "Wave"};
    constexpr int ShapeCount = 9;
    const char *MotionNames[] = {"Arc", "Wave", "Circle", "Star", "Zigzag"};
    constexpr int MotionCount = 5;
    const char *TrackNames[] = {"Move X", "Move Y", "Scale", "Rotate", "Fade (alpha)", "Fill color", "Draw on (trimEnd)", "Erase (trimStart)", "Morph"};

    std::string Num(float v)
    {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.2f", v);
        std::string s = buffer;
        while (!s.empty() && s.back() == '0')
            s.pop_back();
        if (!s.empty() && s.back() == '.')
            s.pop_back();
        if (s == "-0")
            s = "0";
        return s;
    }

    SDL_Color ToColor(const float *v)
    {
        auto b = [](float x)
        { return static_cast<Uint8>(std::clamp(x, 0.0f, 255.0f)); };
        return SDL_Color{b(v[0]), b(v[1]), b(v[2]), 255};
    }
}

TweenLabPanel::TweenLabPanel()
{
    m_CurveNames.push_back("Custom cubic");
    for (const std::string &name : SIMPEngine::Curve::Names())
        m_CurveNames.push_back(name);
    for (size_t i = 0; i < m_CurveNames.size(); ++i)
        if (m_CurveNames[i] == "easeOutBack")
            m_Curve = static_cast<int>(i);

    auto set = [&](TrackId id, bool on, float from, float to)
    {
        m_Tracks[id].Enabled = on;
        m_Tracks[id].From[0] = from;
        m_Tracks[id].To[0] = to;
    };
    set(MoveX, true, -160.0f, 160.0f);
    set(MoveY, false, -80.0f, 80.0f);
    set(Scale, false, 0.3f, 1.0f);
    set(Rotation, true, 0.0f, 360.0f);
    set(Alpha, false, 0.0f, 255.0f);
    set(Draw, false, 0.0f, 1.0f);
    set(Erase, false, 0.0f, 1.0f);
    set(Morph, false, 0.0f, 1.0f);
    float from[4] = {104, 136, 255, 255};
    float to[4] = {255, 118, 170, 255};
    std::copy(from, from + 4, m_Tracks[FillColor].From);
    std::copy(to, to + 4, m_Tracks[FillColor].To);
}

TweenLabPanel::~TweenLabPanel() = default;

SIMPEngine::VectorPath TweenLabPanel::ShapePath(int index)
{
    using SIMPEngine::VectorPath;
    switch (index)
    {
    case 0:
        return VectorPath::Circle(40);
    case 1:
        return VectorPath::Rect(80, 80);
    case 2:
        return VectorPath::Rect(80, 80, 22);
    case 3:
        return VectorPath::Star(5, 50, 22);
    case 4:
        return VectorPath::Heart(90);
    case 5:
        return VectorPath::Polygon(6, 46);
    case 6:
        return VectorPath::Points({{-40, -12}, {10, -12}, {10, -28}, {44, 0}, {10, 28}, {10, 12}, {-40, 12}}, true);
    case 7:
        return VectorPath::Line(-60, 0, 60, 0);
    default:
    {
        VectorPath p;
        p.MoveTo(-70, 0).QuadTo(-52, 30, -35, 0).QuadTo(-17, -30, 0, 0).QuadTo(17, 30, 35, 0).QuadTo(52, -30, 70, 0);
        return p;
    }
    }
}

const char *TweenLabPanel::ShapeCode(int index)
{
    static const char *codes[] = {
        "Path.Circle(40)",
        "Path.Rect(80, 80)",
        "Path.Rect(80, 80, 22)",
        "Path.Star(5, 50, 22)",
        "Path.Heart(90)",
        "Path.Polygon(6, 46)",
        "Path.Points({-40, -12, 10, -12, 10, -28, 44, 0, 10, 28, 10, 12, -40, 12}, true)",
        "Path.Line(-60, 0, 60, 0)",
        "Path.New():MoveTo(-70, 0):QuadTo(-52, 30, -35, 0):QuadTo(-17, -30, 0, 0):QuadTo(17, 30, 35, 0):QuadTo(52, -30, 70, 0)",
    };
    return codes[std::clamp(index, 0, ShapeCount - 1)];
}

SIMPEngine::VectorPath TweenLabPanel::MotionPath(int index)
{
    using SIMPEngine::VectorPath;
    VectorPath p;
    switch (index)
    {
    case 0:
        p.MoveTo(-200, -90).CubicTo(-120, 150, 120, 150, 200, -90);
        return p;
    case 1:
        p.MoveTo(-220, 0).QuadTo(-165, 110, -110, 0).QuadTo(-55, -110, 0, 0).QuadTo(55, 110, 110, 0).QuadTo(165, -110, 220, 0);
        return p;
    case 2:
        return VectorPath::Circle(150);
    case 3:
        return VectorPath::Star(5, 170, 75);
    default:
        return VectorPath::Points({{-200, -80}, {-100, 80}, {0, -80}, {100, 80}, {200, -80}}, false);
    }
}

const char *TweenLabPanel::MotionCode(int index)
{
    static const char *codes[] = {
        "Path.New():MoveTo(-200, -90):CubicTo(-120, 150, 120, 150, 200, -90)",
        "Path.New():MoveTo(-220, 0):QuadTo(-165, 110, -110, 0):QuadTo(-55, -110, 0, 0):QuadTo(55, 110, 110, 0):QuadTo(165, -110, 220, 0)",
        "Path.Circle(150)",
        "Path.Star(5, 170, 75)",
        "Path.Points({-200, -80, -100, 80, 0, -80, 100, 80, 200, -80}, false)",
    };
    return codes[std::clamp(index, 0, MotionCount - 1)];
}

bool TweenLabPanel::CurveAt(SIMPEngine::Curve &curve) const
{
    if (m_PreviewCurve > 0)
        return SIMPEngine::Curve::FromName(m_CurveNames[static_cast<size_t>(m_PreviewCurve)], curve);
    if (m_Curve == 0)
    {
        curve = SIMPEngine::Curve::Cubic(m_Cubic[0], m_Cubic[1], m_Cubic[2], m_Cubic[3]);
        return true;
    }
    return SIMPEngine::Curve::FromName(m_CurveNames[static_cast<size_t>(m_Curve)], curve);
}

std::string TweenLabPanel::CurveCode() const
{
    if (m_Curve == 0)
        return "Curves.Cubic(" + Num(m_Cubic[0]) + ", " + Num(m_Cubic[1]) + ", " + Num(m_Cubic[2]) + ", " + Num(m_Cubic[3]) + ")";
    return "\"" + m_CurveNames[static_cast<size_t>(m_Curve)] + "\"";
}

void TweenLabPanel::EnsureScene()
{
    if (m_Scene)
        return;
    m_Scene = std::make_unique<SIMPEngine::Scene>("Tween Lab");
    m_Object = m_Scene->CreateEntity("Thing");
    m_Object.AddComponent<VectorShapeComponent>();
}

std::vector<uint32_t> TweenLabPanel::ActiveIds() const
{
    std::vector<uint32_t> ids;
    for (uint32_t id : {m_PropertyId, m_MotionId, m_SpringId})
        if (id)
            ids.push_back(id);
    return ids;
}

void TweenLabPanel::ApplyStyle()
{
    auto &v = m_Object.GetComponent<VectorShapeComponent>();
    v.path = ShapePath(m_Shape);
    v.morphTarget.Clear();
    v.morph = 0.0f;
    v.fill = m_Fill;
    v.fillColor = m_FillColor;
    v.stroke = m_Stroke;
    v.strokeColor = m_StrokeColor;
    v.strokeWidth = m_StrokeWidth;
    v.cap = SIMPEngine::StrokeCap::Round;
    v.join = SIMPEngine::StrokeJoin::Round;
    v.trimStart = 0.0f;
    v.trimEnd = 1.0f;
    v.trimOffset = 0.0f;
}

void TweenLabPanel::Restart()
{
    using namespace SIMPEngine;
    EnsureScene();
    TweenSystem &tweens = m_Scene->GetTweens();
    for (uint32_t id : ActiveIds())
        tweens.Release(id);
    tweens.Clear();
    m_PropertyId = m_MotionId = m_SpringId = 0;

    auto &t = m_Object.GetComponent<TransformComponent>();
    t.position = {0.0f, 0.0f};
    t.rotation = 0.0f;
    t.scale = {1.0f, 1.0f};
    ApplyStyle();
    if (m_Tracks[Morph].Enabled)
        m_Object.GetComponent<VectorShapeComponent>().morphTarget = ShapePath(m_MorphShape);

    std::vector<TweenTrack> tracks;
    auto add = [&](TrackId id, TweenProperty property, int channels, const float *from, const float *to)
    {
        if (!m_Tracks[id].Enabled || (m_Along && (id == MoveX || id == MoveY)))
            return;
        TweenTrack track;
        track.Property = property;
        track.Channels = channels;
        track.ExplicitFrom = true;
        for (int c = 0; c < 4; ++c)
        {
            track.From[c] = from[channels == 2 ? 0 : c];
            track.To[c] = to[channels == 2 ? 0 : c];
        }
        tracks.push_back(track);
    };
    const float zero[4] = {0, 0, 0, 0};
    const float one[4] = {1, 1, 1, 1};
    add(MoveX, TweenProperty::X, 1, m_Tracks[MoveX].From, m_Tracks[MoveX].To);
    add(MoveY, TweenProperty::Y, 1, m_Tracks[MoveY].From, m_Tracks[MoveY].To);
    add(Scale, TweenProperty::Scale, 2, m_Tracks[Scale].From, m_Tracks[Scale].To);
    add(Rotation, TweenProperty::Rotation, 1, m_Tracks[Rotation].From, m_Tracks[Rotation].To);
    add(Alpha, TweenProperty::Alpha, 1, m_Tracks[Alpha].From, m_Tracks[Alpha].To);
    add(FillColor, TweenProperty::FillColor, 4, m_Tracks[FillColor].From, m_Tracks[FillColor].To);
    add(Draw, TweenProperty::TrimEnd, 1, zero, one);
    add(Erase, TweenProperty::TrimStart, 1, zero, one);
    add(Morph, TweenProperty::Morph, 1, zero, one);

    const entt::entity handle = m_Object.GetHandle();
    for (const TweenTrack &track : tracks)
        tweens.Write(handle, track.Property, track.From);

    Curve curve;
    CurveAt(curve);
    auto configure = [&](uint32_t id)
    {
        if (TweenNode *node = tweens.Get(id))
        {
            node->Delay = m_Delay;
            node->Repeat = m_LoopMode == 0 ? 0 : m_LoopMode == 1 ? std::max(1, m_LoopCount) : -1;
            node->Yoyo = m_Yoyo;
            node->Speed = m_Speed;
        }
        tweens.AddRef(id);
    };

    if (!tracks.empty())
    {
        if (m_Spring)
        {
            SpringParams params;
            params.Stiffness = m_Stiffness;
            params.Damping = m_Damping;
            params.Mass = m_Mass;
            m_SpringId = tweens.Spring(handle, tracks, params);
            tweens.AddRef(m_SpringId);
        }
        else
        {
            m_PropertyId = tweens.To(handle, tracks, m_Duration, curve);
            configure(m_PropertyId);
        }
    }

    if (m_Along)
    {
        std::vector<FlatContour> outline;
        MotionPath(m_MotionPath).Flatten(0.5f, outline);
        if (!outline.empty() && !outline.front().Points.empty())
        {
            t.position = outline.front().Points.front();
            m_MotionId = tweens.Motion(handle, outline.front().Points, outline.front().Closed, m_Duration, curve, m_Orient, false);
            configure(m_MotionId);
        }
    }

    m_Trail.clear();
    m_Paused = false;
    m_Idle = 0.0f;
    m_Dirty = false;
}

bool TweenLabPanel::AllFinished() const
{
    const SIMPEngine::TweenSystem &tweens = m_Scene->GetTweens();
    for (uint32_t id : ActiveIds())
    {
        const SIMPEngine::TweenNode *node = tweens.Get(id);
        if (node && !node->Finished && !node->Dead)
            return false;
    }
    return true;
}

float TweenLabPanel::CycleProgress() const
{
    if (!m_Scene)
        return 0.0f;
    const SIMPEngine::TweenNode *node = m_Scene->GetTweens().Get(m_PropertyId ? m_PropertyId : m_MotionId);
    if (!node || node->Duration <= 0.0f)
        return 0.0f;
    const float local = node->Time - node->Delay;
    if (local <= 0.0f)
        return 0.0f;
    const float d = node->Duration;
    const int cycles = node->Repeat < 0 ? 1 << 30 : node->Repeat + 1;
    int cycle;
    float within;
    if (local >= d * static_cast<float>(cycles))
    {
        cycle = cycles - 1;
        within = d;
    }
    else
    {
        cycle = static_cast<int>(local / d);
        within = local - static_cast<float>(cycle) * d;
    }
    const bool forward = !(node->Yoyo && cycle % 2 == 1);
    const float p = within / d;
    return forward ? p : 1.0f - p;
}

void TweenLabPanel::UpdatePreview(float dt)
{
    if (m_Dirty || !m_Scene)
        Restart();
    if (!m_Paused)
        m_Scene->GetTweens().Update(dt);

    const glm::vec2 position = m_Object.GetComponent<TransformComponent>().position;
    if (m_Trail.empty() || glm::length(m_Trail.back() - position) > 3.0f)
    {
        m_Trail.push_back(position);
        if (m_Trail.size() > 90)
            m_Trail.erase(m_Trail.begin());
    }

    if (m_AutoReplay && !m_Paused && AllFinished())
    {
        m_Idle += dt;
        if (m_Idle > 0.7f)
            Restart();
    }
}

void TweenLabPanel::RenderPreview(int width, int height)
{
    using namespace SIMPEngine;
    width = std::max(8, width);
    height = std::max(8, height);
    if (!m_Target)
        m_Target = std::make_unique<Framebuffer>(width, height);
    else if (m_Target->GetWidth() != width || m_Target->GetHeight() != height)
        m_Target->Resize(width, height);

    const float viewH = 480.0f;
    const float viewW = viewH * static_cast<float>(width) / static_cast<float>(height);
    const glm::mat4 proj = glm::ortho(-viewW * 0.5f, viewW * 0.5f, -viewH * 0.5f, viewH * 0.5f, -100000.0f, 100000.0f);

    const ImVec4 &bg = EditorUI::Colors::Canvas;
    Renderer::BeginRenderTarget(*m_Target);
    Renderer::SetClearColor(bg.x * 0.75f, bg.y * 0.75f, bg.z * 0.8f, 1.0f);
    Renderer::Clear();
    Renderer::BeginScene(glm::mat4(1.0f), proj);

    const SDL_Color grid{255, 255, 255, 14};
    const SDL_Color axis{255, 255, 255, 34};
    for (int x = -2000; x <= 2000; x += 50)
        Renderer::DrawLine(static_cast<float>(x), -viewH, static_cast<float>(x), viewH, x == 0 ? axis : grid, -10.0f);
    for (int y = -1000; y <= 1000; y += 50)
        Renderer::DrawLine(-viewW, static_cast<float>(y), viewW, static_cast<float>(y), y == 0 ? axis : grid, -10.0f);

    if (m_Along && m_ShowPath)
    {
        std::vector<FlatContour> outline;
        MotionPath(m_MotionPath).Flatten(0.5f, outline);
        StrokeStyle style;
        style.Width = 2.0f;
        for (const FlatContour &c : outline)
            VectorShapes::DrawPolyline(c.Points, c.Closed, style, glm::vec4(1.0f, 1.0f, 1.0f, 0.18f), -6.0f);
    }

    if (m_ShowTrail)
    {
        const size_t n = m_Trail.size();
        for (size_t i = 0; i < n; ++i)
        {
            const float a = static_cast<float>(i + 1) / static_cast<float>(n);
            Renderer::DrawCircle(m_Trail[i].x, m_Trail[i].y, 3.0f, SDL_Color{255, 214, 90, static_cast<Uint8>(40 + 140 * a)}, 1.0f, -5.0f);
        }
    }

    m_Scene->OnRender(glm::mat4(1.0f), proj);
    Renderer::EndScene();
    Renderer::EndRenderTarget();
}

std::string TweenLabPanel::GenerateCode() const
{
    std::ostringstream out;
    out << "local e = Scene.Spawn(\"Thing\", 0, 0)\n";
    out << "local v = e:Add(\"Vector\")\n";
    out << "v:SetPath(" << ShapeCode(m_Shape) << ")\n";
    if (m_Fill)
        out << "v:SetFill(" << int(m_FillColor.r) << ", " << int(m_FillColor.g) << ", " << int(m_FillColor.b) << ")\n";
    else
        out << "v:SetFillEnabled(false)\n";
    if (m_Stroke)
        out << "v:SetStroke(" << int(m_StrokeColor.r) << ", " << int(m_StrokeColor.g) << ", " << int(m_StrokeColor.b) << ")\nv:SetStrokeWidth(" << Num(m_StrokeWidth) << ")\n";
    if (m_Tracks[Morph].Enabled)
        out << "v:SetMorphTarget(" << ShapeCode(m_MorphShape) << ")\n";

    std::vector<std::pair<std::string, std::string>> props;
    auto color = [](const float *c)
    { return "{" + Num(c[0]) + ", " + Num(c[1]) + ", " + Num(c[2]) + "}"; };
    if (m_Tracks[MoveX].Enabled && !m_Along)
        props.push_back({"x = " + Num(m_Tracks[MoveX].From[0]), "x = " + Num(m_Tracks[MoveX].To[0])});
    if (m_Tracks[MoveY].Enabled && !m_Along)
        props.push_back({"y = " + Num(m_Tracks[MoveY].From[0]), "y = " + Num(m_Tracks[MoveY].To[0])});
    if (m_Tracks[Scale].Enabled)
    {
        const std::string a = Num(m_Tracks[Scale].From[0]), b = Num(m_Tracks[Scale].To[0]);
        props.push_back({"scale = {" + a + ", " + a + "}", "scale = {" + b + ", " + b + "}"});
    }
    if (m_Tracks[Rotation].Enabled)
        props.push_back({"rotation = " + Num(m_Tracks[Rotation].From[0]), "rotation = " + Num(m_Tracks[Rotation].To[0])});
    if (m_Tracks[Alpha].Enabled)
        props.push_back({"alpha = " + Num(m_Tracks[Alpha].From[0]), "alpha = " + Num(m_Tracks[Alpha].To[0])});
    if (m_Tracks[FillColor].Enabled)
        props.push_back({"fillColor = " + color(m_Tracks[FillColor].From), "fillColor = " + color(m_Tracks[FillColor].To)});
    if (m_Tracks[Draw].Enabled)
        props.push_back({"trimEnd = 0", "trimEnd = 1"});
    if (m_Tracks[Erase].Enabled)
        props.push_back({"trimStart = 0", "trimStart = 1"});
    if (m_Tracks[Morph].Enabled)
        props.push_back({"morph = 0", "morph = 1"});

    std::vector<std::string> opts;
    if (m_Delay > 0.0f)
        opts.push_back("delay = " + Num(m_Delay));
    if (m_LoopMode == 1)
        opts.push_back("loop = " + std::to_string(std::max(1, m_LoopCount)));
    else if (m_LoopMode == 2)
        opts.push_back("loop = true");
    if (m_Yoyo)
        opts.push_back("yoyo = true");
    if (m_Speed != 1.0f)
        opts.push_back("speed = " + Num(m_Speed));
    std::string optText;
    for (size_t i = 0; i < opts.size(); ++i)
        optText += (i ? ", " : "") + opts[i];

    auto join = [&](bool to)
    {
        std::string s;
        for (size_t i = 0; i < props.size(); ++i)
            s += (i ? ", " : "") + (to ? props[i].second : props[i].first);
        return "{" + s + "}";
    };

    if (!props.empty())
    {
        if (m_Spring)
        {
            out << "Tween.FromTo(e, " << join(false) << ", " << join(false) << ", 0)\n";
            out << "Tween.Spring(e, " << join(true) << ", {stiffness = " << Num(m_Stiffness) << ", damping = " << Num(m_Damping) << ", mass = " << Num(m_Mass) << "})\n";
        }
        else
        {
            out << "Tween.FromTo(e,\n    " << join(false) << ",\n    " << join(true) << ",\n    " << Num(m_Duration) << ", " << CurveCode();
            if (!optText.empty())
                out << ",\n    {" << optText << "}";
            out << ")\n";
        }
    }
    if (m_Along)
    {
        std::string alongOpts = optText;
        if (m_Orient)
            alongOpts = "orient = true" + std::string(alongOpts.empty() ? "" : ", ") + alongOpts;
        out << "Tween.Along(e, " << MotionCode(m_MotionPath) << ", " << Num(m_Duration) << ", " << CurveCode();
        if (!alongOpts.empty())
            out << ", {" << alongOpts << "}";
        out << ")\n";
    }
    if (props.empty() && !m_Along)
        out << "-- turn on a track to animate something\n";
    return out.str();
}

void TweenLabPanel::DrawTracks()
{
    using namespace EditorUI;
    SectionLabel("What moves");
    for (int i = 0; i < TrackCount; ++i)
    {
        Track &track = m_Tracks[i];
        ImGui::PushID(i);
        const bool blocked = m_Along && (i == MoveX || i == MoveY);
        ImGui::BeginDisabled(blocked);
        if (PropertyBool(TrackNames[i], &track.Enabled))
            m_Dirty = true;
        if (track.Enabled)
        {
            if (i == FillColor)
            {
                SDL_Color from = ToColor(track.From);
                SDL_Color to = ToColor(track.To);
                if (PropertyColor("  From", from))
                {
                    track.From[0] = from.r;
                    track.From[1] = from.g;
                    track.From[2] = from.b;
                    m_Dirty = true;
                }
                if (PropertyColor("  To", to))
                {
                    track.To[0] = to.r;
                    track.To[1] = to.g;
                    track.To[2] = to.b;
                    m_Dirty = true;
                }
            }
            else if (i == Morph)
            {
                PropertyLabel("  Into");
                if (ImGui::Combo("##into", &m_MorphShape, ShapeNames, ShapeCount - 2))
                    m_Dirty = true;
            }
            else if (i != Draw && i != Erase)
            {
                PropertyLabel("  From / To");
                float values[2] = {track.From[0], track.To[0]};
                const float speed = i == Scale ? 0.01f : 1.0f;
                if (ImGui::DragFloat2("##ft", values, speed, 0.0f, 0.0f, i == Scale ? "%.2f" : "%.0f"))
                {
                    track.From[0] = values[0];
                    track.To[0] = values[1];
                    m_Dirty = true;
                }
            }
        }
        ImGui::EndDisabled();
        if (blocked && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("The path moves the object, so Move X/Y are off while Follow a path is on");
        ImGui::PopID();
    }

    if (PropertyBool("Follow a path", &m_Along))
        m_Dirty = true;
    if (m_Along)
    {
        PropertyLabel("  Path");
        if (ImGui::Combo("##path", &m_MotionPath, MotionNames, MotionCount))
            m_Dirty = true;
        if (PropertyBool("  Face direction", &m_Orient))
            m_Dirty = true;
    }
}

void TweenLabPanel::DrawTiming()
{
    using namespace EditorUI;
    SectionLabel("How it moves");
    if (PropertyBool("Spring (physics)", &m_Spring))
        m_Dirty = true;
    if (m_Spring)
    {
        m_Dirty |= PropertyFloat("Stiffness", &m_Stiffness, 1.0f, 1.0f, 2000.0f, "%.0f");
        m_Dirty |= PropertyFloat("Damping", &m_Damping, 0.2f, 0.0f, 200.0f, "%.1f");
        m_Dirty |= PropertyFloat("Mass", &m_Mass, 0.02f, 0.05f, 20.0f, "%.2f");
        if (m_Along)
            ImGui::TextDisabled("The path still uses the curve below");
    }
    if (!m_Spring || m_Along)
    {
        PropertyLabel("Curve");
        int hovered = -1;
        if (ImGui::BeginCombo("##curve", m_CurveNames[static_cast<size_t>(m_Curve)].c_str(), ImGuiComboFlags_HeightLarge))
        {
            for (size_t i = 0; i < m_CurveNames.size(); ++i)
            {
                const bool selected = static_cast<int>(i) == m_Curve;
                if (ImGui::Selectable(m_CurveNames[i].c_str(), selected))
                {
                    m_Curve = static_cast<int>(i);
                    m_Dirty = true;
                }
                if (ImGui::IsItemHovered())
                    hovered = static_cast<int>(i);
                if (selected && ImGui::IsWindowAppearing())
                    ImGui::SetScrollHereY();
            }
            ImGui::EndCombo();
        }
        if (hovered != m_PreviewCurve)
        {
            m_PreviewCurve = hovered;
            m_Dirty = true;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Hover a curve in the list to preview it, click to keep it");
        if (m_Curve == 0)
        {
            PropertyLabel("  x1 y1 x2 y2");
            if (ImGui::DragFloat4("##cubic", m_Cubic, 0.005f, -1.0f, 2.0f, "%.2f"))
            {
                m_Cubic[0] = std::clamp(m_Cubic[0], 0.0f, 1.0f);
                m_Cubic[2] = std::clamp(m_Cubic[2], 0.0f, 1.0f);
                m_Dirty = true;
            }
        }
        m_Dirty |= PropertyFloat("Duration", &m_Duration, 0.01f, 0.05f, 10.0f, "%.2f s");
    }
    m_Dirty |= PropertyFloat("Delay", &m_Delay, 0.01f, 0.0f, 5.0f, "%.2f s");
    if (!m_Spring || m_Along)
    {
        static const char *loops[] = {"Once", "Times", "Forever"};
        m_Dirty |= PropertySegmented("Repeat", &m_LoopMode, loops, 3);
        if (m_LoopMode == 1)
            m_Dirty |= PropertyInt("  Extra plays", &m_LoopCount, 0.1f, 1, 20);
        m_Dirty |= PropertyBool("Yoyo (go back)", &m_Yoyo);
        m_Dirty |= PropertyFloat("Speed", &m_Speed, 0.01f, 0.05f, 5.0f, "%.2fx");
    }
}

void TweenLabPanel::DrawControls()
{
    using namespace EditorUI;
    SectionLabel("Thing");
    PropertyLabel("Shape");
    if (ImGui::Combo("##shape", &m_Shape, ShapeNames, ShapeCount))
    {
        m_Dirty = true;
        if (m_Shape >= 7)
        {
            m_Fill = false;
            m_Stroke = true;
            m_StrokeWidth = std::max(m_StrokeWidth, 8.0f);
        }
    }
    m_Dirty |= PropertyBool("Fill", &m_Fill);
    if (m_Fill)
        m_Dirty |= PropertyColor("  Fill color", m_FillColor);
    m_Dirty |= PropertyBool("Outline", &m_Stroke);
    if (m_Stroke)
    {
        m_Dirty |= PropertyColor("  Outline color", m_StrokeColor);
        m_Dirty |= PropertyFloat("  Width", &m_StrokeWidth, 0.1f, 0.5f, 60.0f, "%.1f");
    }

    DrawTracks();
    DrawTiming();

    SectionLabel("Preview");
    PropertyBool("Auto replay", &m_AutoReplay);
    PropertyBool("Show trail", &m_ShowTrail);
    PropertyBool("Show path", &m_ShowPath);
}

void TweenLabPanel::DrawTransport()
{
    using namespace EditorUI;
    SIMPEngine::TweenSystem &tweens = m_Scene->GetTweens();

    if (IconButton("##playpause", m_Paused ? ICON_LC_PLAY : ICON_LC_PAUSE, m_Paused ? "Play" : "Pause"))
    {
        m_Paused = !m_Paused;
        for (uint32_t id : ActiveIds())
        {
            if (m_Paused)
                tweens.Pause(id);
            else
                tweens.Play(id);
        }
    }
    ImGui::SameLine();
    if (IconButton("##restart", ICON_LC_ROTATE_CCW, "Restart"))
        Restart();
    ImGui::SameLine();
    if (IconButton("##reverse", ICON_LC_ARROW_LEFT_RIGHT, "Reverse (play backwards from here)"))
    {
        m_Paused = false;
        m_Idle = 0.0f;
        for (uint32_t id : ActiveIds())
            tweens.Reverse(id);
    }
    ImGui::SameLine();

    const bool canScrub = m_PropertyId || m_MotionId;
    float progress = CycleProgress();
    if (canScrub && !(m_LoopMode == 2))
    {
        const SIMPEngine::TweenNode *node = tweens.Get(m_PropertyId ? m_PropertyId : m_MotionId);
        progress = node ? tweens.Progress(node->Id) : 0.0f;
    }
    ImGui::SetNextItemWidth(-90.0f);
    ImGui::BeginDisabled(!canScrub || m_LoopMode == 2);
    m_Scrub = progress;
    if (ImGui::SliderFloat("##scrub", &m_Scrub, 0.0f, 1.0f, "scrub %.2f"))
    {
        m_Paused = true;
        for (uint32_t id : {m_PropertyId, m_MotionId})
            if (id)
            {
                tweens.Pause(id);
                tweens.Seek(id, m_Scrub);
            }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled(m_Spring ? "spring" : (m_Paused ? "paused" : "playing"));
}

void TweenLabPanel::DrawCurveGraph(float width, float height)
{
    using namespace EditorUI;
    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + width, min.y + height);
    dl->AddRectFilled(min, max, U32(Colors::Frame), Metrics::FrameRounding);
    ImGui::Dummy(ImVec2(width, height));

    if (m_Spring && !m_Along)
    {
        const char *label = "spring: no fixed curve";
        const ImVec2 size = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(min.x + (width - size.x) * 0.5f, min.y + (height - size.y) * 0.5f), U32(Colors::TextFaint), label);
        return;
    }

    const float pad = 10.0f;
    const float vmin = -0.35f, vmax = 1.35f;
    auto map = [&](float t, float v)
    {
        return ImVec2(min.x + pad + t * (width - pad * 2.0f), max.y - pad - (v - vmin) / (vmax - vmin) * (height - pad * 2.0f));
    };
    dl->AddLine(map(0, 0), map(1, 0), U32(Colors::Border));
    dl->AddLine(map(0, 1), map(1, 1), U32(Colors::Border));
    dl->AddLine(map(0, 0), map(1, 1), U32(Colors::TextFaint, 0.35f));

    SIMPEngine::Curve curve;
    CurveAt(curve);
    ImVec2 points[81];
    for (int i = 0; i <= 80; ++i)
    {
        const float t = static_cast<float>(i) / 80.0f;
        points[i] = map(t, curve.Evaluate(t));
    }
    dl->AddPolyline(points, 81, U32(Colors::Accent), ImDrawFlags_None, 2.5f);
    const float p = CycleProgress();
    const ImVec2 marker = map(p, curve.Evaluate(p));
    dl->AddLine(ImVec2(marker.x, min.y + 4), ImVec2(marker.x, max.y - 4), U32(Colors::TextFaint, 0.4f));
    dl->AddCircleFilled(marker, 5.0f, IM_COL32(255, 255, 255, 255));
}

void TweenLabPanel::DrawCode()
{
    using namespace EditorUI;
    std::string code = GenerateCode();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::PushFont(GetFont(FontRole::Mono), 0.0f);
    ImGui::InputTextMultiline("##code", &code, ImVec2(avail.x, avail.y - ImGui::GetFrameHeightWithSpacing()), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopFont();
    if (Button("Copy Lua", ButtonKind::Primary, ImVec2(120, 0)))
        ImGui::SetClipboardText(code.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("paste into any script's OnCreate");
}

void TweenLabPanel::OnImGuiRender()
{
    if (!m_Visible)
        return;

    if (ImGuiWindow *viewport = ImGui::FindWindowByName("Viewport###Viewport"); viewport && viewport->DockId)
        ImGui::SetNextWindowDockID(viewport->DockId, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Tween Lab###TweenLab", &m_Visible))
    {
        ImGui::End();
        return;
    }

    EditorUI::PushEditSuppression();
    UpdatePreview(std::min(ImGui::GetIO().DeltaTime, 0.05f));

    ImGui::BeginChild("##controls", ImVec2(360.0f, 0.0f), ImGuiChildFlags_None);
    DrawControls();
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("##stage", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float bottom = 190.0f;
    const ImVec2 size(avail.x, std::max(120.0f, avail.y - bottom));
    const float scale = ImGui::GetIO().DisplayFramebufferScale.x;
    RenderPreview(static_cast<int>(size.x * scale), static_cast<int>(size.y * scale));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddImageRounded(EditorUI::TextureId(m_Target->GetColorAttachment()), origin, ImVec2(origin.x + size.x, origin.y + size.y),
                                                ImVec2(0, 1), ImVec2(1, 0), IM_COL32_WHITE, EditorUI::Metrics::FrameRounding);
    ImGui::Dummy(size);

    DrawTransport();
    const float graphW = 230.0f;
    ImGui::BeginGroup();
    DrawCurveGraph(graphW, ImGui::GetContentRegionAvail().y);
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    DrawCode();
    ImGui::EndGroup();
    ImGui::EndChild();

    EditorUI::PopEditSuppression();
    ImGui::End();
}
