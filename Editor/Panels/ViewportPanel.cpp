#define IMGUI_DEFINE_MATH_OPERATORS
#include "ViewportPanel.h"
#include "../EditorCommands.h"
#include "UI/Visuals.h"
#include "UI/Widgets.h"
#include <Engine/Core/Log.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Systems/ParticleSystem.h>
#include <Engine/Math/EntityUtility.h>

#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    constexpr float HandleLength = 72.0f;
    constexpr float RotateRadius = 58.0f;
    constexpr float HitTolerance = 6.0f;

    SDL_Color ToSDL(const ImVec4 &c, float alpha = 1.0f)
    {
        return {static_cast<Uint8>(c.x * 255.0f), static_cast<Uint8>(c.y * 255.0f), static_cast<Uint8>(c.z * 255.0f), static_cast<Uint8>(c.w * alpha * 255.0f)};
    }

    float DistanceToSegment(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b)
    {
        ImVec2 ab = b - a;
        float len2 = ab.x * ab.x + ab.y * ab.y;
        float t = len2 > 0.0f ? std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / len2, 0.0f, 1.0f) : 0.0f;
        ImVec2 d = p - (a + ab * t);
        return std::sqrt(d.x * d.x + d.y * d.y);
    }

    float SnapTo(float v, float step)
    {
        return step > 0.0f ? std::round(v / step) * step : v;
    }
}

ViewportPanel::ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer, EditorContext *context)
    : m_TileEditor(context)
{
    m_RenderingLayer = renderingLayer;
    m_Context = context;
}

void ViewportPanel::OnAttach()
{
}

ImVec2 ViewportPanel::ToScreen(const glm::vec2 &world) const
{
    glm::vec2 s = m_Camera.WorldToScreen(world);
    return ImVec2(m_CanvasPos.x + s.x, m_CanvasPos.y + s.y);
}

glm::vec2 ViewportPanel::GetMouseWorldPosition() const
{
    ImVec2 mouse = ImGui::GetIO().MousePos;
    return m_Camera.ScreenToWorld({mouse.x - m_CanvasPos.x, mouse.y - m_CanvasPos.y});
}

bool ViewportPanel::SelectedHasTilemap() const
{
    SIMPEngine::Entity entity = m_Context->SelectedEntity;
    return entity && m_Context->Scene->GetRegistry().valid(entity.GetHandle()) && entity.HasComponent<TilemapComponent>();
}

void ViewportPanel::ViewBounds(glm::vec2 &min, glm::vec2 &max) const
{
    glm::vec2 a = m_Camera.ScreenToWorld({0.0f, 0.0f});
    glm::vec2 b = m_Camera.ScreenToWorld({m_CanvasSize.x, m_CanvasSize.y});
    min = glm::min(a, b);
    max = glm::max(a, b);
}

bool ViewportPanel::SnapActive() const
{
    return EditorUI::Settings().Snap != ImGui::GetIO().KeyCtrl;
}

void ViewportPanel::OnImGuiRender()
{
    PROFILE_SCOPE("Viewport/Render");
    using namespace EditorUI;

    if (m_Context->FocusSceneView)
    {
        ImGui::SetNextWindowFocus();
        m_Context->FocusSceneView = false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool visible = ImGui::Begin("Viewport###Viewport");
    ImGui::PopStyleVar();

    m_Context->ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    m_Context->ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    if (!visible)
    {
        ImGui::End();
        return;
    }

    DrawToolbar();

    m_CanvasPos = ImGui::GetCursorScreenPos();
    m_CanvasSize = ImGui::GetContentRegionAvail();
    if (m_CanvasSize.x < 2.0f || m_CanvasSize.y < 2.0f)
    {
        ImGui::End();
        return;
    }

    RenderScene();

    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddImage(EditorUI::TextureId(m_Framebuffer->GetColorAttachment()), m_CanvasPos, m_CanvasPos + m_CanvasSize, ImVec2(0, 1), ImVec2(1, 0));

    ImGui::InvisibleButton("##canvas", m_CanvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    m_CanvasHovered = ImGui::IsItemHovered();

    HandleDrop();
    HandleCameraInput();
    if (m_TileEditor.IsActive())
    {
        glm::vec2 viewMin, viewMax;
        ViewBounds(viewMin, viewMax);
        m_Context->HoveredEntity = {};
        m_Drag.Active = false;
        m_Marquee.Active = false;
        m_TileEditor.Update(GetMouseWorldPosition(), viewMin, viewMax, m_CanvasHovered, m_Panning);
    }
    else
    {
        HandleSelectionAndGizmo();
    }
    HandleShortcuts();

    dl->PushClipRect(m_CanvasPos, m_CanvasPos + m_CanvasSize, true);
    DrawOverlays();
    if (m_TileEditor.IsActive())
        m_TileEditor.DrawScreen(dl, [this](const glm::vec2 &world)
                                { return ToScreen(world); });
    dl->PopClipRect();

    ImGui::End();
}

void ViewportPanel::DrawToolbar()
{
    using namespace EditorUI;
    EditorSettings &settings = Settings();

    BeginToolbar("##viewport_toolbar");

    bool tileMode = m_TileEditor.IsActive();
    if (SelectedHasTilemap() && m_Context->IsEditing())
    {
        if (IconButton("##tilemode", ICON_LC_PAINTBRUSH, tileMode ? "Stop painting tiles (T)" : "Paint tiles (T)", tileMode))
        {
            m_Context->Tiles.Enabled = !m_Context->Tiles.Enabled;
            if (m_Context->Tiles.Enabled)
                m_Context->Tiles.FocusPalette = true;
        }
        ImGui::SameLine();
        VerticalSeparator();
        ImGui::SameLine();
    }

    if (tileMode)
    {
        DrawTileToolbar();
        EndToolbar();
        return;
    }

    struct ToolDef
    {
        GizmoTool Tool;
        const char *Icon;
        const char *Tip;
    };
    const ToolDef tools[] = {
        {GizmoTool::Select, ICON_LC_MOUSE_POINTER_2, "Select (Q)"},
        {GizmoTool::Move, ICON_LC_MOVE, "Move (W)"},
        {GizmoTool::Rotate, ICON_LC_ROTATE_CW, "Rotate (E)"},
        {GizmoTool::Scale, ICON_LC_SCALING, "Scale (R)"},
    };
    for (const auto &t : tools)
    {
        ImGui::PushID(static_cast<int>(t.Tool));
        if (IconButton("##tool", t.Icon, t.Tip, m_Context->Tool == t.Tool))
            m_Context->Tool = t.Tool;
        ImGui::PopID();
        ImGui::SameLine();
    }

    VerticalSeparator();
    ImGui::SameLine();
    if (IconButton("##snap", ICON_LC_MAGNET, "Snapping (hold Ctrl to invert)", settings.Snap))
    {
        settings.Snap = !settings.Snap;
        ImGui::MarkIniSettingsDirty();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(58.0f);
    if (ImGui::DragFloat("##snapstep", &settings.SnapMove, 0.5f, 0.5f, 1024.0f, "%.0f px"))
        ImGui::MarkIniSettingsDirty();
    Tooltip("Snap step");
    ImGui::SameLine();
    if (IconButton("##grid", ICON_LC_GRID_3X3, "Grid", settings.ShowGrid))
    {
        settings.ShowGrid = !settings.ShowGrid;
        ImGui::MarkIniSettingsDirty();
    }
    ImGui::SameLine();
    if (IconButton("##lighting", ICON_LC_SUN, "Preview lighting and bloom", settings.PreviewLighting))
    {
        settings.PreviewLighting = !settings.PreviewLighting;
        ImGui::MarkIniSettingsDirty();
    }
    ImGui::SameLine();
    if (IconButton("##colliders", ICON_LC_BOX, "Show collision boxes", settings.ShowColliders))
    {
        settings.ShowColliders = !settings.ShowColliders;
        ImGui::MarkIniSettingsDirty();
    }

    char zoom[16];
    std::snprintf(zoom, sizeof(zoom), "%.0f%%", m_Camera.GetZoom() * 100.0f);
    float zoomW = ImGui::CalcTextSize("0000%").x + 12.0f;
    float rightX = ImGui::GetWindowContentRegionMax().x - zoomW - ImGui::GetFrameHeight() * 2.0f - 6.0f;
    ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 8.0f, rightX));
    if (Button(zoom, ButtonKind::Ghost, ImVec2(zoomW, 0)))
        m_Camera.SetZoom(1.0f);
    Tooltip("Reset zoom to 100%");
    ImGui::SameLine();
    if (IconButton("##frame", ICON_LC_SCAN, "Frame selection (F)"))
        FocusOnSelectedEntity();
    ImGui::SameLine();
    if (IconButton("##frameall", ICON_LC_MAXIMIZE, "Frame scene"))
        FrameScene();

    EndToolbar();
}

void ViewportPanel::DrawTileToolbar()
{
    using namespace EditorUI;
    TileEditState &tiles = m_Context->Tiles;

    struct ToolDef
    {
        TileTool Tool;
        const char *Icon;
        const char *Tip;
    };
    const ToolDef tools[] = {
        {TileTool::Brush, ICON_LC_BRUSH, "Brush (B)  Right-click erases"},
        {TileTool::Rect, ICON_LC_SQUARE_DASHED, "Rectangle (R)"},
        {TileTool::Fill, ICON_LC_PAINT_BUCKET, "Bucket fill (G)"},
        {TileTool::Erase, ICON_LC_ERASER, "Eraser (E)"},
        {TileTool::Picker, ICON_LC_PIPETTE, "Pick tile (I or Alt+click)"},
    };
    for (const auto &t : tools)
    {
        ImGui::PushID(static_cast<int>(t.Tool));
        if (IconButton("##tiletool", t.Icon, t.Tip, tiles.Tool == t.Tool))
            tiles.Tool = t.Tool;
        ImGui::PopID();
        ImGui::SameLine();
    }

    VerticalSeparator();
    ImGui::SameLine();
    if (IconButton("##flipx", ICON_LC_FLIP_HORIZONTAL, "Flip horizontally (X)", (tiles.Flags & TileFlipX) != 0))
        tiles.Flags ^= TileFlipX;
    ImGui::SameLine();
    if (IconButton("##flipy", ICON_LC_FLIP_VERTICAL, "Flip vertically (Y)", (tiles.Flags & TileFlipY) != 0))
        tiles.Flags ^= TileFlipY;
    ImGui::SameLine();
    if (IconButton("##grid", ICON_LC_GRID_3X3, "Grid", Settings().ShowGrid))
    {
        Settings().ShowGrid = !Settings().ShowGrid;
        ImGui::MarkIniSettingsDirty();
    }
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Middle or Space+drag to pan");
}

void ViewportPanel::RenderScene()
{
    int w = static_cast<int>(m_CanvasSize.x);
    int h = static_cast<int>(m_CanvasSize.y);

    if (!m_Framebuffer)
        m_Framebuffer = std::make_unique<SIMPEngine::Framebuffer>(w, h);
    else
        m_Framebuffer->Resize(w, h);

    m_Camera.SetViewport(static_cast<float>(w), static_cast<float>(h));
    if (!m_CameraInitialized)
    {
        m_Camera.SetZoom(static_cast<float>(h) / 1300.0f);
        m_CameraInitialized = true;
    }

    SIMPEngine::Scene &scene = *m_Context->Scene;

    SIMPEngine::Renderer::ResetStats();
    SIMPEngine::Renderer::BeginRenderTarget(*m_Framebuffer);

    const glm::vec4 &bg = scene.GetBackgroundColor();
    SIMPEngine::Renderer::SetClearColor(bg.r, bg.g, bg.b, bg.a);
    SIMPEngine::Renderer::Clear();

    glm::mat4 view = m_Camera.GetView();
    glm::mat4 proj = m_Camera.GetProjection();
    SIMPEngine::Renderer::BeginScene(view, proj);
    if (EditorUI::Settings().ShowGrid)
        DrawGrid();

    scene.SetLightingVisible(EditorUI::Settings().PreviewLighting);
    scene.OnRender(view, proj);
    scene.SetLightingVisible(true);
    DrawCameraFrame();
    DrawEmitterShapes();
    DrawLightShapes();
    if (EditorUI::Settings().ShowColliders)
        DrawColliders();
    DrawLogicShapes();
    m_TileEditor.RenderWorld(SIMPEngine::Renderer::GetWorldPerPixel());

    SIMPEngine::Renderer::EndScene();
    m_Stats = SIMPEngine::Renderer::GetStats();
    SIMPEngine::Renderer::EndRenderTarget();
}

void ViewportPanel::DrawGrid()
{
    SIMPEngine::Scene &scene = *m_Context->Scene;
    const glm::vec4 &bg = scene.GetBackgroundColor();
    float luminance = bg.r * 0.299f + bg.g * 0.587f + bg.b * 0.114f;
    SDL_Color minor = luminance > 0.6f ? SDL_Color{0, 0, 0, 22} : SDL_Color{255, 255, 255, 16};
    SDL_Color major = luminance > 0.6f ? SDL_Color{0, 0, 0, 44} : SDL_Color{255, 255, 255, 34};

    glm::vec2 a = m_Camera.ScreenToWorld({0.0f, 0.0f});
    glm::vec2 b = m_Camera.ScreenToWorld({m_CanvasSize.x, m_CanvasSize.y});
    float left = std::min(a.x, b.x), right = std::max(a.x, b.x);
    float bottom = std::min(a.y, b.y), top = std::max(a.y, b.y);

    float step = std::max(1.0f, EditorUI::Settings().GridSize);
    while (step * m_Camera.GetZoom() < 10.0f)
        step *= 2.0f;

    const float z = -100000.0f;
    long first = (long)std::floor(left / step);
    long last = (long)std::ceil(right / step);
    for (long i = first; i <= last && i - first < 600; ++i)
    {
        float x = i * step;
        SIMPEngine::Renderer::DrawLine(x, bottom, x, top, (i % 4 == 0) ? major : minor, z);
    }
    first = (long)std::floor(bottom / step);
    last = (long)std::ceil(top / step);
    for (long i = first; i <= last && i - first < 600; ++i)
    {
        float y = i * step;
        SIMPEngine::Renderer::DrawLine(left, y, right, y, (i % 4 == 0) ? major : minor, z);
    }

    SIMPEngine::Renderer::DrawLine(left, 0.0f, right, 0.0f, ToSDL(EditorUI::Colors::AxisX, 0.55f), z + 1.0f);
    SIMPEngine::Renderer::DrawLine(0.0f, bottom, 0.0f, top, ToSDL(EditorUI::Colors::AxisY, 0.55f), z + 1.0f);
}

void ViewportPanel::DrawCameraFrame()
{
    SIMPEngine::Scene &scene = *m_Context->Scene;
    auto &camera = scene.GetActiveCamera();
    auto [vw, vh] = camera.GetViewportSize();
    float aspect = (vw > 0 && vh > 0) ? static_cast<float>(vw) / static_cast<float>(vh) : 16.0f / 9.0f;
    float hh = 540.0f * camera.GetZoom();
    float hw = hh * aspect;
    glm::vec2 p = camera.GetPosition();

    SIMPEngine::Renderer::DrawRect(p.x, p.y, hw * 2.0f, hh * 2.0f, SIMPEngine::Renderer::GetWorldPerPixel() * 1.5f,
                                   ToSDL(EditorUI::Colors::CameraFrame, 0.9f), 0.0f, 100000.0f);
}

void ViewportPanel::DrawEmitterShapes()
{
    using SIMPEngine::Renderer;
    auto &registry = m_Context->Scene->GetRegistry();
    float wpp = Renderer::GetWorldPerPixel();
    float z = 99995.0f;
    SDL_Color color = ToSDL(EditorUI::Colors::Accent, 0.85f);
    SDL_Color faint = ToSDL(EditorUI::Colors::Accent, 0.4f);

    for (entt::entity handle : m_Context->SelectedHandles())
    {
        if (!registry.valid(handle) || !registry.all_of<ParticleEmitterComponent, TransformComponent>(handle))
            continue;

        const auto &e = registry.get<ParticleEmitterComponent>(handle).effect;
        auto frame = SIMPEngine::ParticleSystem::FrameOf(registry.get<TransformComponent>(handle));
        glm::vec2 o = frame.Origin;
        float r = glm::radians(frame.Rotation);
        glm::vec2 ax(std::cos(r), std::sin(r));

        if (e.Emitter == SIMPEngine::EmitterShape::Circle)
            Renderer::DrawRing(o.x, o.y, e.Radius * frame.Scale, wpp * 1.5f, color, 1.0f, z);
        else if (e.Emitter == SIMPEngine::EmitterShape::Box)
            Renderer::DrawRect(o.x, o.y, e.BoxSize.x * frame.Scale, e.BoxSize.y * frame.Scale, wpp * 1.5f, color, frame.Rotation, z);
        else if (e.Emitter == SIMPEngine::EmitterShape::Line)
        {
            glm::vec2 half = ax * e.BoxSize.x * 0.5f * frame.Scale;
            Renderer::DrawLine(o.x - half.x, o.y - half.y, o.x + half.x, o.y + half.y, wpp * 1.5f, color, z);
        }

        float length = wpp * 56.0f;
        auto ray = [&](float degrees, SDL_Color c)
        {
            float a = glm::radians(degrees + frame.Rotation);
            glm::vec2 tip = o + glm::vec2(std::cos(a), std::sin(a)) * length;
            Renderer::DrawLine(o.x, o.y, tip.x, tip.y, wpp * 1.5f, c, z + 1.0f);
            return tip;
        };
        glm::vec2 tip = ray(e.Direction, color);
        Renderer::DrawCircle(tip.x, tip.y, wpp * 3.0f, color, 1.0f, z + 1.0f);
        if (e.Spread > 0.5f && e.Spread < 179.5f)
        {
            ray(e.Direction - e.Spread, faint);
            ray(e.Direction + e.Spread, faint);
        }
    }
}

void ViewportPanel::DrawLightShapes()
{
    using SIMPEngine::Renderer;
    auto &registry = m_Context->Scene->GetRegistry();
    float wpp = Renderer::GetWorldPerPixel();
    float z = 99995.0f;

    for (auto entity : registry.view<LightComponent, TransformComponent>())
    {
        const auto &light = registry.get<LightComponent>(entity);
        auto world = SIMPEngine::DecomposeWorld(registry.get<TransformComponent>(entity));
        bool selected = m_Context->IsSelected(entity);
        SDL_Color tint{light.color.r, light.color.g, light.color.b, static_cast<Uint8>(selected ? 230 : 150)};

        Renderer::DrawRing(world.Position.x, world.Position.y, wpp * 7.0f, wpp * 2.0f, tint, 1.0f, z);
        Renderer::DrawCircle(world.Position.x, world.Position.y, wpp * 3.0f, tint, 1.0f, z);
        if (!selected)
            continue;

        float scale = (std::abs(world.Scale.x) + std::abs(world.Scale.y)) * 0.5f;
        float radius = light.radius * scale;
        SDL_Color faint = ToSDL(EditorUI::Colors::Accent, 0.55f);
        if (light.type == LightType::Point)
        {
            Renderer::DrawRing(world.Position.x, world.Position.y, radius, wpp * 1.5f, faint, 1.0f, z);
            continue;
        }

        auto ray = [&](float degrees)
        {
            float a = glm::radians(degrees + world.Rotation);
            glm::vec2 tip = world.Position + glm::vec2(std::cos(a), std::sin(a)) * radius;
            Renderer::DrawLine(world.Position.x, world.Position.y, tip.x, tip.y, wpp * 1.5f, faint, z);
        };
        ray(-light.outerAngle);
        ray(light.outerAngle);
        ray(0.0f);
        int segments = std::max(4, static_cast<int>(light.outerAngle / 6.0f));
        for (int i = 0; i < segments; ++i)
        {
            float a0 = glm::radians(world.Rotation - light.outerAngle + 2.0f * light.outerAngle * static_cast<float>(i) / static_cast<float>(segments));
            float a1 = glm::radians(world.Rotation - light.outerAngle + 2.0f * light.outerAngle * static_cast<float>(i + 1) / static_cast<float>(segments));
            glm::vec2 p0 = world.Position + glm::vec2(std::cos(a0), std::sin(a0)) * radius;
            glm::vec2 p1 = world.Position + glm::vec2(std::cos(a1), std::sin(a1)) * radius;
            Renderer::DrawLine(p0.x, p0.y, p1.x, p1.y, wpp * 1.5f, faint, z);
        }
    }
}

void ViewportPanel::DrawColliders()
{
    using SIMPEngine::Renderer;
    auto &registry = m_Context->Scene->GetRegistry();
    float wpp = Renderer::GetWorldPerPixel();
    SDL_Color color = ToSDL(EditorUI::Colors::Success, 0.85f);

    for (auto entity : registry.view<CollisionComponent, TransformComponent>())
    {
        const auto &collision = registry.get<CollisionComponent>(entity);
        auto world = SIMPEngine::DecomposeWorld(registry.get<TransformComponent>(entity));
        glm::vec2 size(collision.width * std::abs(world.Scale.x), collision.height * std::abs(world.Scale.y));
        glm::vec2 center = world.Position + glm::vec2(collision.offsetX, collision.offsetY) + size * 0.5f;
        Renderer::DrawRect(center.x, center.y, size.x, size.y, wpp * 1.5f, color, 0.0f, 99994.0f);
    }

    for (auto entity : registry.view<PhysicsComponent, TransformComponent>())
    {
        const auto &physics = registry.get<PhysicsComponent>(entity);
        if (!physics.body)
            continue;
        auto world = SIMPEngine::DecomposeWorld(registry.get<TransformComponent>(entity));
        if (physics.body->shapeType == AccelEngine::ShapeType::CIRCLE)
            Renderer::DrawRing(world.Position.x, world.Position.y, physics.body->circle.radius, wpp * 1.5f, color, 1.0f, 99994.0f);
        else
            Renderer::DrawRect(world.Position.x, world.Position.y, physics.body->aabb.halfSize.x * 2.0f, physics.body->aabb.halfSize.y * 2.0f, wpp * 1.5f, color, world.Rotation, 99994.0f);
    }
}

void ViewportPanel::DrawLogicShapes()
{
    using SIMPEngine::Renderer;
    auto &registry = m_Context->Scene->GetRegistry();
    float wpp = Renderer::GetWorldPerPixel();
    const bool showAll = EditorUI::Settings().ShowColliders;

    for (auto entity : registry.view<TriggerComponent, TransformComponent>())
    {
        if (!showAll && !m_Context->IsSelected(entity))
            continue;

        const auto &trigger = registry.get<TriggerComponent>(entity);
        auto world = SIMPEngine::DecomposeWorld(registry.get<TransformComponent>(entity));
        glm::vec2 size(trigger.width * std::abs(world.Scale.x), trigger.height * std::abs(world.Scale.y));
        glm::vec2 center = world.Position + glm::vec2(trigger.offsetX, trigger.offsetY) + size * 0.5f;
        SDL_Color color = ToSDL(EditorUI::Colors::Warning, trigger.enabled ? 0.9f : 0.35f);
        Renderer::DrawRect(center.x, center.y, size.x, size.y, wpp * 1.5f, color, 0.0f, 99994.0f);
    }

    SDL_Color controllerColor = ToSDL(EditorUI::Colors::Accent, 0.9f);
    for (auto entity : registry.view<CharacterControllerComponent, TransformComponent>())
    {
        if (!showAll && !m_Context->IsSelected(entity))
            continue;

        const auto &controller = registry.get<CharacterControllerComponent>(entity);
        auto world = SIMPEngine::DecomposeWorld(registry.get<TransformComponent>(entity));
        glm::vec2 center = world.Position + controller.offset;
        glm::vec2 size = glm::abs(controller.size);
        Renderer::DrawRect(center.x, center.y, size.x, size.y, wpp * 1.5f, controllerColor, 0.0f, 99994.0f);
        if (controller.mode == ControllerMode::Platformer)
            Renderer::DrawLine(center.x - size.x * 0.3f, center.y - size.y * 0.5f, center.x + size.x * 0.3f, center.y - size.y * 0.5f,
                               wpp * 3.0f, controllerColor, 99994.0f);
    }

    SDL_Color pathColor = ToSDL(EditorUI::Colors::Accent, 0.85f);
    for (entt::entity handle : m_Context->SelectedHandles())
    {
        if (!registry.valid(handle) || !registry.all_of<PathFollowerComponent, TransformComponent>(handle))
            continue;

        const auto &path = registry.get<PathFollowerComponent>(handle);
        auto world = SIMPEngine::DecomposeWorld(registry.get<TransformComponent>(handle));
        glm::vec2 previous = world.Position;
        for (const glm::vec2 &point : path.points)
        {
            glm::vec2 target = path.relative ? world.Position + point : point;
            Renderer::DrawLine(previous.x, previous.y, target.x, target.y, wpp * 1.5f, pathColor, 99994.0f);
            Renderer::DrawCircle(target.x, target.y, wpp * 4.0f, pathColor, 1.0f, 99995.0f);
            previous = target;
        }
    }
}

void ViewportPanel::HandleCameraInput()
{
    ImGuiIO &io = ImGui::GetIO();
    glm::vec2 local(io.MousePos.x - m_CanvasPos.x, io.MousePos.y - m_CanvasPos.y);

    if (m_CanvasHovered && io.MouseWheel != 0.0f)
        m_Camera.ZoomAt(io.MouseWheel, local);

    bool rightPans = !m_TileEditor.IsActive();
    bool spacePan = ImGui::IsKeyDown(ImGuiKey_Space) && m_CanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    if (m_CanvasHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) || (rightPans && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) || spacePan))
        m_Panning = true;

    if (m_Panning)
    {
        bool held = ImGui::IsMouseDown(ImGuiMouseButton_Middle) || (rightPans && ImGui::IsMouseDown(ImGuiMouseButton_Right)) ||
                    (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsKeyDown(ImGuiKey_Space));
        if (held)
            m_Camera.Pan({io.MouseDelta.x, io.MouseDelta.y});
        else
            m_Panning = false;
    }
}

SIMPEngine::Entity ViewportPanel::PickEntity(const glm::vec2 &world) const
{
    auto &registry = m_Context->Scene->GetRegistry();
    SIMPEngine::Entity best{};
    float bestZ = -FLT_MAX;

    for (auto handle : registry.view<TransformComponent, TagComponent>())
    {
        SIMPEngine::Entity entity(handle, m_Context->Scene);
        if (!SIMPEngine::PointInEntityBounds(entity, world))
            continue;

        float z = registry.get<TransformComponent>(handle).zIndex;
        if (z >= bestZ)
        {
            bestZ = z;
            best = entity;
        }
    }
    return best;
}

ViewportPanel::Handle ViewportPanel::HitTestGizmo(SIMPEngine::Entity entity, const ImVec2 &mouse) const
{
    auto &tc = entity.GetComponent<TransformComponent>();
    SIMPEngine::WorldTransform2D w = SIMPEngine::DecomposeWorld(tc);
    ImVec2 c = ToScreen(w.Position);
    ImVec2 d = mouse - c;
    float dist = std::sqrt(d.x * d.x + d.y * d.y);

    switch (m_Context->Tool)
    {
    case GizmoTool::Move:
        if (std::abs(d.x) <= 8.0f && std::abs(d.y) <= 8.0f)
            return Handle::Center;
        if (DistanceToSegment(mouse, c, c + ImVec2(HandleLength, 0)) < HitTolerance)
            return Handle::AxisX;
        if (DistanceToSegment(mouse, c, c + ImVec2(0, -HandleLength)) < HitTolerance)
            return Handle::AxisY;
        break;
    case GizmoTool::Rotate:
        if (std::abs(dist - RotateRadius) < HitTolerance + 1.0f)
            return Handle::Rotate;
        break;
    case GizmoTool::Scale:
    {
        float r = glm::radians(w.Rotation);
        ImVec2 ax(std::cos(r), -std::sin(r));
        ImVec2 ay(-std::sin(r), -std::cos(r));
        if (std::abs(d.x) <= 8.0f && std::abs(d.y) <= 8.0f)
            return Handle::ScaleXY;
        if (DistanceToSegment(mouse, c, c + ax * HandleLength) < HitTolerance)
            return Handle::ScaleX;
        if (DistanceToSegment(mouse, c, c + ay * HandleLength) < HitTolerance)
            return Handle::ScaleY;
        break;
    }
    default:
        break;
    }
    return Handle::None;
}

void ViewportPanel::HandleSelectionAndGizmo()
{
    SIMPEngine::Scene &scene = *m_Context->Scene;
    auto &registry = scene.GetRegistry();
    ImGuiIO &io = ImGui::GetIO();

    SIMPEngine::Entity selected = m_Context->SelectedEntity;
    if (selected && (!registry.valid(selected.GetHandle()) || !selected.HasComponent<TransformComponent>()))
        selected = {};

    glm::vec2 mouseWorld = GetMouseWorldPosition();

    m_HoverHandle = Handle::None;
    if (selected && !m_Drag.Active && m_CanvasHovered && !m_Panning)
        m_HoverHandle = HitTestGizmo(selected, io.MousePos);

    m_Context->HoveredEntity = (m_CanvasHovered && !m_Panning) ? PickEntity(mouseWorld) : SIMPEngine::Entity{};

    if (m_CanvasHovered && !m_Panning && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsKeyDown(ImGuiKey_Space))
    {
        Handle grab = m_HoverHandle;
        if (grab == Handle::None)
        {
            SIMPEngine::Entity picked = PickEntity(mouseWorld);
            if (!picked)
            {
                m_Marquee.Active = true;
                m_Marquee.Additive = io.KeyShift;
                m_Marquee.Start = io.MousePos;
            }
            else if (io.KeyShift)
            {
                m_Context->ToggleSelection(picked);
                selected = m_Context->SelectedEntity;
            }
            else
            {
                if (m_Context->IsSelected(picked.GetHandle()))
                    m_Context->MakePrimary(picked);
                else
                    m_Context->Select(picked);
                selected = picked;
                if (m_Context->Tool != GizmoTool::Rotate && m_Context->Tool != GizmoTool::Scale)
                    grab = Handle::Body;
            }
        }

        if (selected && grab != Handle::None)
        {
            scene.UpdateTransforms();
            auto &tc = selected.GetComponent<TransformComponent>();
            SIMPEngine::WorldTransform2D w = SIMPEngine::DecomposeWorld(tc);

            m_Drag = {};
            m_Drag.Active = true;
            m_Drag.Grab = grab;
            m_Drag.StartMouseWorld = mouseWorld;
            m_Drag.StartMouseScreen = {io.MousePos.x, io.MousePos.y};
            m_Drag.StartWorldPos = w.Position;
            m_Drag.StartScale = tc.scale;
            m_Drag.StartRotation = tc.rotation;
            glm::vec2 rel = mouseWorld - w.Position;
            m_Drag.StartAngle = std::atan2(rel.y, rel.x);

            bool moving = grab == Handle::Body || grab == Handle::Center || grab == Handle::AxisX || grab == Handle::AxisY;
            if (moving && m_Context->HasExtraSelection())
            {
                std::vector<entt::entity> roots = m_Context->SelectedRoots();
                bool primaryIsRoot = std::find(roots.begin(), roots.end(), selected.GetHandle()) != roots.end();
                for (entt::entity handle : roots)
                {
                    if (!primaryIsRoot || handle == selected.GetHandle() || !registry.valid(handle) || !registry.all_of<TransformComponent>(handle))
                        continue;
                    SIMPEngine::WorldTransform2D other = SIMPEngine::DecomposeWorld(registry.get<TransformComponent>(handle));
                    m_Drag.Others.emplace_back(handle, other.Position);
                }
            }
        }
    }

    if (m_Marquee.Active)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            m_Marquee.Active = false;
        else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            FinishMarquee();
    }

    if (m_Drag.Active)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && selected)
            ApplyDrag(selected);
        else
            m_Drag.Active = false;
    }
}

void ViewportPanel::ApplyDrag(SIMPEngine::Entity entity)
{
    ImGuiIO &io = ImGui::GetIO();
    glm::vec2 screenDelta(io.MousePos.x - m_Drag.StartMouseScreen.x, io.MousePos.y - m_Drag.StartMouseScreen.y);
    if (!m_Drag.Recorded && std::abs(screenDelta.x) + std::abs(screenDelta.y) < 3.0f)
        return;

    if (!m_Drag.Recorded)
    {
        EditorCommands::RecordEdit(*m_Context);
        m_Drag.Recorded = true;
    }

    SIMPEngine::Scene &scene = *m_Context->Scene;
    auto &registry = scene.GetRegistry();
    auto &tc = entity.GetComponent<TransformComponent>();
    glm::vec2 mouseWorld = GetMouseWorldPosition();
    glm::vec2 delta = mouseWorld - m_Drag.StartMouseWorld;
    bool snap = SnapActive();
    const EditorUI::EditorSettings &settings = EditorUI::Settings();

    auto setWorldPosition = [&](entt::entity handle, glm::vec2 target)
    {
        auto &t = registry.get<TransformComponent>(handle);
        glm::vec4 local(target, 0.0f, 1.0f);
        if (auto *h = registry.try_get<HierarchyComponent>(handle);
            h && h->parent != entt::null && registry.valid(h->parent) && registry.all_of<TransformComponent>(h->parent))
            local = glm::inverse(registry.get<TransformComponent>(h->parent).worldTransform) * glm::vec4(target, 0.0f, 1.0f);
        t.position = {local.x, local.y};
        t.dirty = true;
    };

    switch (m_Drag.Grab)
    {
    case Handle::Body:
    case Handle::Center:
    case Handle::AxisX:
    case Handle::AxisY:
    {
        glm::vec2 target = m_Drag.StartWorldPos;
        if (m_Drag.Grab != Handle::AxisY)
            target.x += delta.x;
        if (m_Drag.Grab != Handle::AxisX)
            target.y += delta.y;
        if (snap)
        {
            if (m_Drag.Grab != Handle::AxisY)
                target.x = SnapTo(target.x, settings.SnapMove);
            if (m_Drag.Grab != Handle::AxisX)
                target.y = SnapTo(target.y, settings.SnapMove);
        }
        glm::vec2 offset = target - m_Drag.StartWorldPos;
        setWorldPosition(entity.GetHandle(), target);
        for (const auto &[handle, start] : m_Drag.Others)
        {
            if (registry.valid(handle) && registry.all_of<TransformComponent>(handle))
                setWorldPosition(handle, start + offset);
        }
        break;
    }
    case Handle::Rotate:
    {
        glm::vec2 rel = mouseWorld - m_Drag.StartWorldPos;
        float angle = std::atan2(rel.y, rel.x);
        float rotation = m_Drag.StartRotation + glm::degrees(angle - m_Drag.StartAngle);
        if (snap)
            rotation = SnapTo(rotation, settings.SnapRotate);
        tc.rotation = rotation;
        break;
    }
    case Handle::ScaleX:
    case Handle::ScaleY:
    case Handle::ScaleXY:
    {
        SIMPEngine::WorldTransform2D w = SIMPEngine::DecomposeWorld(tc);
        float r = glm::radians(w.Rotation);
        glm::vec2 ax(std::cos(r), std::sin(r));
        glm::vec2 ay(-std::sin(r), std::cos(r));
        glm::vec2 start = m_Drag.StartMouseWorld - m_Drag.StartWorldPos;
        glm::vec2 now = mouseWorld - m_Drag.StartWorldPos;

        auto ratio = [](float a, float b)
        { return std::abs(a) > 1e-4f ? b / a : 1.0f; };

        glm::vec2 factor(1.0f);
        if (m_Drag.Grab == Handle::ScaleX)
            factor.x = ratio(glm::dot(start, ax), glm::dot(now, ax));
        else if (m_Drag.Grab == Handle::ScaleY)
            factor.y = ratio(glm::dot(start, ay), glm::dot(now, ay));
        else
        {
            float f = ratio(glm::length(start), glm::length(now));
            factor = {f, f};
        }

        glm::vec2 scale = m_Drag.StartScale * factor;
        if (snap)
            scale = {SnapTo(scale.x, 0.1f), SnapTo(scale.y, 0.1f)};
        tc.scale = scale;
        break;
    }
    default:
        break;
    }

    tc.dirty = true;
    scene.UpdateTransforms();
    if (m_Context->IsEditing())
        scene.SyncPhysicsFromTransforms();
}

void ViewportPanel::HandleShortcuts()
{
    if (!(m_Context->ViewportFocused || m_CanvasHovered) || ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive())
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_T, false) && SelectedHasTilemap() && m_Context->IsEditing())
    {
        m_Context->Tiles.Enabled = !m_Context->Tiles.Enabled;
        if (m_Context->Tiles.Enabled)
            m_Context->Tiles.FocusPalette = true;
    }

    if (m_TileEditor.IsActive())
    {
        m_TileEditor.HandleShortcuts();
        if (ImGui::IsKeyPressed(ImGuiKey_F, false))
            FocusOnSelectedEntity();
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Q, false))
        m_Context->Tool = GizmoTool::Select;
    if (ImGui::IsKeyPressed(ImGuiKey_W, false))
        m_Context->Tool = GizmoTool::Move;
    if (ImGui::IsKeyPressed(ImGuiKey_E, false))
        m_Context->Tool = GizmoTool::Rotate;
    if (ImGui::IsKeyPressed(ImGuiKey_R, false))
        m_Context->Tool = GizmoTool::Scale;
    if (ImGui::IsKeyPressed(ImGuiKey_F, false))
        FocusOnSelectedEntity();
}

void ViewportPanel::HandleDrop()
{
    if (!ImGui::BeginDragDropTarget())
        return;

    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("TEXTURE_ASSET"))
    {
        std::string vpath(static_cast<const char *>(payload->Data));
        glm::vec2 world = GetMouseWorldPosition();
        if (EditorUI::Settings().Snap)
            world = {SnapTo(world.x, EditorUI::Settings().SnapMove), SnapTo(world.y, EditorUI::Settings().SnapMove)};
        if (auto entity = EditorCommands::CreateSprite(*m_Context, vpath, world))
            m_Context->SelectedEntity = entity;
    }
    ImGui::EndDragDropTarget();
}

std::vector<entt::entity> ViewportPanel::EntitiesInMarquee() const
{
    std::vector<entt::entity> hits;
    ImVec2 a = m_Marquee.Start;
    ImVec2 b = ImGui::GetIO().MousePos;
    ImVec2 lo(std::min(a.x, b.x), std::min(a.y, b.y));
    ImVec2 hi(std::max(a.x, b.x), std::max(a.y, b.y));

    auto &registry = m_Context->Scene->GetRegistry();
    for (auto handle : registry.view<TransformComponent, TagComponent>())
    {
        SIMPEngine::Entity entity(handle, m_Context->Scene);
        auto corners = SIMPEngine::GetEntityCorners(entity);
        if (!corners)
            continue;

        ImVec2 mn(FLT_MAX, FLT_MAX);
        ImVec2 mx(-FLT_MAX, -FLT_MAX);
        for (const glm::vec2 &corner : *corners)
        {
            ImVec2 p = ToScreen(corner);
            mn = ImVec2(std::min(mn.x, p.x), std::min(mn.y, p.y));
            mx = ImVec2(std::max(mx.x, p.x), std::max(mx.y, p.y));
        }

        if (mx.x >= lo.x && mn.x <= hi.x && mx.y >= lo.y && mn.y <= hi.y)
            hits.push_back(handle);
    }
    return hits;
}

void ViewportPanel::FinishMarquee()
{
    ImVec2 d = ImGui::GetIO().MousePos - m_Marquee.Start;
    bool dragged = std::abs(d.x) + std::abs(d.y) >= 4.0f;
    bool additive = m_Marquee.Additive;
    m_Marquee.Active = false;

    if (!dragged)
    {
        if (!additive)
            m_Context->ClearSelection();
        return;
    }

    std::vector<entt::entity> hits = EntitiesInMarquee();
    if (!additive)
    {
        m_Context->SetSelection(hits);
        return;
    }

    std::vector<entt::entity> handles = m_Context->SelectedHandles();
    for (entt::entity handle : hits)
    {
        if (std::find(handles.begin(), handles.end(), handle) == handles.end())
            handles.push_back(handle);
    }
    m_Context->SetSelection(handles);
}

void ViewportPanel::DrawOutline(SIMPEngine::Entity entity, ImU32 color, float thickness)
{
    auto corners = SIMPEngine::GetEntityCorners(entity);
    if (!corners)
        return;

    ImVec2 pts[4];
    for (int i = 0; i < 4; ++i)
        pts[i] = ToScreen((*corners)[i]);
    ImGui::GetWindowDrawList()->AddPolyline(pts, 4, color, ImDrawFlags_Closed, thickness);
}

void ViewportPanel::DrawGizmo(SIMPEngine::Entity entity)
{
    using namespace EditorUI;
    ImDrawList *dl = ImGui::GetWindowDrawList();

    auto &tc = entity.GetComponent<TransformComponent>();
    SIMPEngine::WorldTransform2D w = SIMPEngine::DecomposeWorld(tc);
    ImVec2 c = ImFloor(ToScreen(w.Position)) + ImVec2(0.5f, 0.5f);

    Handle active = m_Drag.Active ? m_Drag.Grab : m_HoverHandle;
    auto tone = [&](Handle h, const ImVec4 &base)
    { return active == h ? U32(Colors::Text) : U32(base); };

    auto arrow = [&](ImVec2 dir, Handle h, const ImVec4 &color)
    {
        ImVec2 tip = c + dir * HandleLength;
        ImVec2 n(-dir.y, dir.x);
        dl->AddLine(c, tip - dir * 8.0f, tone(h, color), 2.0f);
        dl->AddTriangleFilled(tip, tip - dir * 11.0f + n * 5.0f, tip - dir * 11.0f - n * 5.0f, tone(h, color));
    };

    switch (m_Context->Tool)
    {
    case GizmoTool::Move:
        arrow(ImVec2(1, 0), Handle::AxisX, Colors::AxisX);
        arrow(ImVec2(0, -1), Handle::AxisY, Colors::AxisY);
        dl->AddRectFilled(c - ImVec2(5, 5), c + ImVec2(5, 5), tone(Handle::Center, Colors::Accent));
        break;
    case GizmoTool::Rotate:
    {
        dl->AddCircle(c, RotateRadius, tone(Handle::Rotate, Colors::Accent), 64, 2.0f);
        float r = glm::radians(w.Rotation);
        ImVec2 knob = c + ImVec2(std::cos(r), -std::sin(r)) * RotateRadius;
        dl->AddLine(c, knob, U32(Colors::Accent, 0.5f), 1.0f);
        dl->AddCircleFilled(knob, 4.0f, tone(Handle::Rotate, Colors::Accent));
        dl->AddCircleFilled(c, 2.5f, U32(Colors::Text));
        break;
    }
    case GizmoTool::Scale:
    {
        float r = glm::radians(w.Rotation);
        ImVec2 ax(std::cos(r), -std::sin(r));
        ImVec2 ay(-std::sin(r), -std::cos(r));
        ImVec2 ex = c + ax * HandleLength;
        ImVec2 ey = c + ay * HandleLength;
        dl->AddLine(c, ex, tone(Handle::ScaleX, Colors::AxisX), 2.0f);
        dl->AddLine(c, ey, tone(Handle::ScaleY, Colors::AxisY), 2.0f);
        dl->AddRectFilled(ex - ImVec2(4.5f, 4.5f), ex + ImVec2(4.5f, 4.5f), tone(Handle::ScaleX, Colors::AxisX));
        dl->AddRectFilled(ey - ImVec2(4.5f, 4.5f), ey + ImVec2(4.5f, 4.5f), tone(Handle::ScaleY, Colors::AxisY));
        dl->AddRectFilled(c - ImVec2(5, 5), c + ImVec2(5, 5), tone(Handle::ScaleXY, Colors::Accent));
        break;
    }
    default:
        dl->AddCircleFilled(c, 2.5f, U32(Colors::Accent));
        break;
    }
}

void ViewportPanel::DrawOverlays()
{
    using namespace EditorUI;
    ImDrawList *dl = ImGui::GetWindowDrawList();
    auto &registry = m_Context->Scene->GetRegistry();

    SIMPEngine::Entity hovered = m_Context->HoveredEntity;
    SIMPEngine::Entity selected = m_Context->SelectedEntity;
    bool selectedValid = selected && registry.valid(selected.GetHandle()) && selected.HasComponent<TransformComponent>();

    if (hovered && registry.valid(hovered.GetHandle()) && !m_Context->IsSelected(hovered.GetHandle()) && !m_Drag.Active && !m_Marquee.Active)
        DrawOutline(hovered, U32(Colors::Accent, 0.5f), 1.0f);

    for (entt::entity handle : m_Context->SelectedHandles())
    {
        if (selectedValid && handle == selected.GetHandle())
            continue;
        if (registry.all_of<TransformComponent>(handle))
            DrawOutline(SIMPEngine::Entity(handle, m_Context->Scene), U32(Colors::Accent, 0.8f), 1.5f);
    }

    if (selectedValid && !m_TileEditor.IsActive())
    {
        DrawOutline(selected, U32(Colors::Accent), 1.5f);
        DrawGizmo(selected);
    }

    if (m_Marquee.Active)
    {
        ImVec2 mouse = ImGui::GetIO().MousePos;
        ImVec2 d = mouse - m_Marquee.Start;
        if (std::abs(d.x) + std::abs(d.y) >= 4.0f)
        {
            ImVec2 lo(std::min(m_Marquee.Start.x, mouse.x), std::min(m_Marquee.Start.y, mouse.y));
            ImVec2 hi(std::max(m_Marquee.Start.x, mouse.x), std::max(m_Marquee.Start.y, mouse.y));
            for (entt::entity handle : EntitiesInMarquee())
                DrawOutline(SIMPEngine::Entity(handle, m_Context->Scene), U32(Colors::Accent, 0.6f), 1.0f);
            dl->AddRectFilled(lo, hi, U32(Colors::Accent, 0.12f));
            dl->AddRect(lo, hi, U32(Colors::Accent, 0.9f), 0.0f, 0, 1.0f);
        }
    }

    float fs = ImGui::GetFontSize();
    ImVec2 bl(m_CanvasPos.x + 8.0f, m_CanvasPos.y + m_CanvasSize.y - fs - 6.0f);
    if (m_CanvasHovered)
    {
        glm::vec2 world = GetMouseWorldPosition();
        char text[64];
        std::snprintf(text, sizeof(text), "%.1f, %.1f", world.x, world.y);
        dl->AddText(bl + ImVec2(1, 1), IM_COL32(0, 0, 0, 120), text);
        dl->AddText(bl, IM_COL32(255, 255, 255, 215), text);
    }

    if (Settings().ShowStats)
    {
        char stats[128];
        std::snprintf(stats, sizeof(stats), "%d draw calls  %d primitives  %d textures", m_Stats.drawCalls, m_Stats.primitives, m_Stats.textureSwitches);
        ImVec2 ts = ImGui::CalcTextSize(stats);
        ImVec2 p(m_CanvasPos.x + m_CanvasSize.x - ts.x - 8.0f, m_CanvasPos.y + 6.0f);
        dl->AddText(p + ImVec2(1, 1), IM_COL32(0, 0, 0, 120), stats);
        dl->AddText(p, IM_COL32(255, 255, 255, 215), stats);
    }

    if (m_Context->State != SceneState::Edit)
    {
        ImVec4 color = m_Context->State == SceneState::Play ? Colors::Success : Colors::Warning;
        dl->AddRect(m_CanvasPos, m_CanvasPos + m_CanvasSize, U32(color, 0.8f), 0.0f, 0, 2.0f);
    }
}

void ViewportPanel::FocusOnSelectedEntity()
{
    if (!m_Context->SelectedEntity || !m_Context->Scene->GetRegistry().valid(m_Context->SelectedEntity.GetHandle()))
        return;

    if (auto bounds = SIMPEngine::GetEntityBounds2D(m_Context->SelectedEntity))
    {
        glm::vec2 size = glm::max(bounds->max - bounds->min, glm::vec2(160.0f));
        m_Camera.Frame((bounds->min + bounds->max) * 0.5f, size);
    }
    else
    {
        m_Camera.SetPosition(SIMPEngine::GetEntityCenter(m_Context->SelectedEntity));
    }
}

void ViewportPanel::FrameScene()
{
    auto &registry = m_Context->Scene->GetRegistry();
    bool any = false;
    glm::vec2 mn(0.0f), mx(0.0f);
    for (auto handle : registry.view<TransformComponent>())
    {
        auto bounds = SIMPEngine::GetEntityBounds2D(SIMPEngine::Entity(handle, m_Context->Scene));
        if (!bounds)
            continue;
        mn = any ? glm::min(mn, bounds->min) : bounds->min;
        mx = any ? glm::max(mx, bounds->max) : bounds->max;
        any = true;
    }

    if (!any)
    {
        m_Camera.SetPosition({0.0f, 0.0f});
        return;
    }
    m_Camera.Frame((mn + mx) * 0.5f, glm::max(mx - mn, glm::vec2(320.0f)) * 1.2f);
}
