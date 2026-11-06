#define IMGUI_DEFINE_MATH_OPERATORS
#include "TileEditor.h"
#include "EditorCommands.h"
#include "UI/Theme.h"

#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/TilemapUtils.h>

#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace
{
    glm::ivec2 Min(const glm::ivec2 &a, const glm::ivec2 &b) { return glm::min(a, b); }
    glm::ivec2 Max(const glm::ivec2 &a, const glm::ivec2 &b) { return glm::max(a, b); }
}

TilemapComponent *TileEditor::Map() const
{
    SIMPEngine::Entity entity = m_Context->SelectedEntity;
    if (!entity || !m_Context->Scene->GetRegistry().valid(entity.GetHandle()))
        return nullptr;
    return m_Context->Scene->GetRegistry().try_get<TilemapComponent>(entity.GetHandle());
}

TransformComponent *TileEditor::Transform() const
{
    SIMPEngine::Entity entity = m_Context->SelectedEntity;
    if (!entity || !m_Context->Scene->GetRegistry().valid(entity.GetHandle()))
        return nullptr;
    return m_Context->Scene->GetRegistry().try_get<TransformComponent>(entity.GetHandle());
}

bool TileEditor::IsActive() const
{
    return m_Context->Tiles.Enabled && m_Context->IsEditing() && Map() && Transform();
}

void TileEditor::Begin(Stroke stroke)
{
    EditorCommands::RecordEdit(*m_Context);
    m_Stroke = stroke;
    m_Last = m_Hover;
    m_RectStart = m_Hover;
}

void TileEditor::End()
{
    if (m_Stroke == Stroke::Rect || m_Stroke == Stroke::RectErase)
        ApplyRect(m_RectStart, m_Hover, m_Stroke == Stroke::RectErase);
    if (TilemapComponent *map = Map(); map && m_Stroke != Stroke::Paint)
        SIMPEngine::PruneEmptyChunks(*map);
    m_Stroke = Stroke::None;
}

void TileEditor::PaintStamp(const glm::ivec2 &cell)
{
    TilemapComponent *map = Map();
    if (!map || !map->tileset)
        return;

    const TileEditState &tiles = m_Context->Tiles;
    for (int sy = 0; sy < tiles.StampHeight; ++sy)
    {
        for (int sx = 0; sx < tiles.StampWidth; ++sx)
        {
            int id = tiles.StampAt(sx, sy);
            if (id >= 0 && map->tileset->IsValid(id))
                SIMPEngine::SetTile(*map, cell.x + sx, cell.y - sy, id, tiles.Flags);
        }
    }
}

void TileEditor::EraseFootprint(const glm::ivec2 &cell)
{
    TilemapComponent *map = Map();
    if (!map)
        return;

    const TileEditState &tiles = m_Context->Tiles;
    for (int sy = 0; sy < tiles.StampHeight; ++sy)
        for (int sx = 0; sx < tiles.StampWidth; ++sx)
            SIMPEngine::EraseTile(*map, cell.x + sx, cell.y - sy);
}

void TileEditor::StrokeLine(const glm::ivec2 &from, const glm::ivec2 &to)
{
    glm::ivec2 d(std::abs(to.x - from.x), -std::abs(to.y - from.y));
    glm::ivec2 s(from.x < to.x ? 1 : -1, from.y < to.y ? 1 : -1);
    int err = d.x + d.y;
    glm::ivec2 p = from;

    for (int guard = 0; guard < 4096; ++guard)
    {
        if (m_Stroke == Stroke::Erase)
            EraseFootprint(p);
        else
            PaintStamp(p);

        if (p == to)
            break;
        int e2 = 2 * err;
        if (e2 >= d.y)
        {
            err += d.y;
            p.x += s.x;
        }
        if (e2 <= d.x)
        {
            err += d.x;
            p.y += s.y;
        }
    }
}

void TileEditor::ApplyRect(const glm::ivec2 &a, const glm::ivec2 &b, bool erase)
{
    TilemapComponent *map = Map();
    if (!map || (!erase && !map->tileset))
        return;

    glm::ivec2 lo = Min(a, b);
    glm::ivec2 hi = Max(a, b);
    if ((hi.x - lo.x + 1) * (hi.y - lo.y + 1) > 262144)
        return;

    const TileEditState &tiles = m_Context->Tiles;
    for (int y = lo.y; y <= hi.y; ++y)
    {
        for (int x = lo.x; x <= hi.x; ++x)
        {
            if (erase)
            {
                SIMPEngine::EraseTile(*map, x, y);
                continue;
            }
            int id = tiles.StampAt(x - lo.x, hi.y - y);
            if (id >= 0 && map->tileset->IsValid(id))
                SIMPEngine::SetTile(*map, x, y, id, tiles.Flags);
        }
    }
}

void TileEditor::ApplyFill(const glm::ivec2 &cell, bool erase)
{
    TilemapComponent *map = Map();
    TransformComponent *transform = Transform();
    if (!map || !transform || (!erase && !map->tileset))
        return;

    glm::ivec2 lo = SIMPEngine::WorldToTile(*map, *transform, m_ViewMin);
    glm::ivec2 hi = SIMPEngine::WorldToTile(*map, *transform, m_ViewMax);
    glm::ivec2 usedMin, usedMax;
    if (SIMPEngine::GetUsedBounds(*map, usedMin, usedMax))
    {
        lo = Min(lo, usedMin - glm::ivec2(1));
        hi = Max(hi, usedMax + glm::ivec2(1));
    }
    hi = Min(hi, lo + glm::ivec2(511));

    int id = erase ? -1 : m_Context->Tiles.StampAt(0, 0);
    if (!erase && (id < 0 || !map->tileset->IsValid(id)))
        return;

    EditorCommands::RecordEdit(*m_Context);
    SIMPEngine::FloodFill(*map, cell, id, erase ? 0 : m_Context->Tiles.Flags, lo, hi);
}

void TileEditor::Pick(const glm::ivec2 &cell)
{
    TilemapComponent *map = Map();
    if (!map)
        return;

    Tile *tile = SIMPEngine::GetTile(*map, cell.x, cell.y);
    if (!tile || tile->id < 0)
        return;

    m_Context->Tiles.SetSingle(tile->id);
    m_Context->Tiles.Flags = tile->flags;
    m_Context->Tiles.Tool = TileTool::Brush;
}

void TileEditor::Update(const glm::vec2 &mouseWorld, const glm::vec2 &viewMin, const glm::vec2 &viewMax, bool hovered, bool panning)
{
    m_ViewMin = viewMin;
    m_ViewMax = viewMax;

    TilemapComponent *map = Map();
    TransformComponent *transform = Transform();
    if (!IsActive() || !map || !transform)
    {
        m_Stroke = Stroke::None;
        m_HoverValid = false;
        return;
    }

    m_Hover = SIMPEngine::WorldToTile(*map, *transform, mouseWorld);
    m_HoverValid = hovered && !panning;

    ImGuiIO &io = ImGui::GetIO();
    TileTool tool = m_Context->Tiles.Tool;

    if (m_Stroke == Stroke::None && m_HoverValid && !ImGui::IsKeyDown(ImGuiKey_Space))
    {
        bool left = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool right = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

        if (left && (io.KeyAlt || tool == TileTool::Picker))
        {
            Pick(m_Hover);
        }
        else if (left || right)
        {
            bool erase = right || tool == TileTool::Erase;
            switch (tool)
            {
            case TileTool::Rect:
                Begin(erase ? Stroke::RectErase : Stroke::Rect);
                break;
            case TileTool::Fill:
                ApplyFill(m_Hover, erase);
                break;
            default:
                Begin(erase ? Stroke::Erase : Stroke::Paint);
                StrokeLine(m_Hover, m_Hover);
                break;
            }
        }
    }

    if (m_Stroke == Stroke::None)
        return;

    bool held = ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right);
    if ((m_Stroke == Stroke::Paint || m_Stroke == Stroke::Erase) && m_Hover != m_Last)
    {
        StrokeLine(m_Last, m_Hover);
        m_Last = m_Hover;
    }

    if (!held)
        End();
    else if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && (m_Stroke == Stroke::Rect || m_Stroke == Stroke::RectErase))
        m_Stroke = Stroke::None;
}

void TileEditor::HandleShortcuts()
{
    if (!IsActive())
        return;

    TileEditState &tiles = m_Context->Tiles;
    if (ImGui::IsKeyPressed(ImGuiKey_B, false))
        tiles.Tool = TileTool::Brush;
    if (ImGui::IsKeyPressed(ImGuiKey_R, false))
        tiles.Tool = TileTool::Rect;
    if (ImGui::IsKeyPressed(ImGuiKey_G, false))
        tiles.Tool = TileTool::Fill;
    if (ImGui::IsKeyPressed(ImGuiKey_E, false))
        tiles.Tool = TileTool::Erase;
    if (ImGui::IsKeyPressed(ImGuiKey_I, false))
        tiles.Tool = TileTool::Picker;
    if (ImGui::IsKeyPressed(ImGuiKey_X, false))
        tiles.Flags ^= TileFlipX;
    if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
        tiles.Flags ^= TileFlipY;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && m_Stroke == Stroke::None)
        tiles.Enabled = false;
}

void TileEditor::RenderWorld(float worldPerPixel)
{
    TilemapComponent *map = Map();
    TransformComponent *transform = Transform();
    if (!IsActive() || !map || !transform)
        return;

    glm::vec2 cell = SIMPEngine::TilemapCellSize(*map, *transform);
    glm::ivec2 lo = SIMPEngine::WorldToTile(*map, *transform, m_ViewMin);
    glm::ivec2 hi = SIMPEngine::WorldToTile(*map, *transform, m_ViewMax) + glm::ivec2(1);
    const float z = 99990.0f;

    if (cell.x / worldPerPixel >= 6.0f && cell.y / worldPerPixel >= 6.0f && hi.x - lo.x < 600 && hi.y - lo.y < 600)
    {
        SDL_Color line{255, 255, 255, 38};
        glm::vec2 a = SIMPEngine::TileToWorld(*map, *transform, lo);
        glm::vec2 b = SIMPEngine::TileToWorld(*map, *transform, hi);
        for (int x = lo.x; x <= hi.x; ++x)
        {
            float wx = SIMPEngine::TileToWorld(*map, *transform, {x, 0}).x;
            SIMPEngine::Renderer::DrawLine(wx, a.y, wx, b.y, line, z);
        }
        for (int y = lo.y; y <= hi.y; ++y)
        {
            float wy = SIMPEngine::TileToWorld(*map, *transform, {0, y}).y;
            SIMPEngine::Renderer::DrawLine(a.x, wy, b.x, wy, line, z);
        }
    }

    if (!m_HoverValid || !map->tileset || !map->tileset->GetTexture())
        return;

    const TileEditState &tiles = m_Context->Tiles;
    bool ghostBrush = m_Stroke == Stroke::None && (tiles.Tool == TileTool::Brush || tiles.Tool == TileTool::Rect) && !ImGui::GetIO().KeyAlt;
    bool ghostRect = m_Stroke == Stroke::Rect;
    if (!ghostBrush && !ghostRect)
        return;

    SIMPEngine::SpriteDesc desc;
    desc.TextureRef = map->tileset->GetTexture();
    desc.Size = cell;
    desc.Z = z + 1.0f;
    desc.Tint = {255, 255, 255, 150};
    desc.FlipX = (tiles.Flags & TileFlipX) != 0;
    desc.FlipY = (tiles.Flags & TileFlipY) != 0;

    auto ghost = [&](const glm::ivec2 &at, int id)
    {
        if (id < 0 || !map->tileset->IsValid(id))
            return;
        SDL_FRect src = map->tileset->GetTileRect(id);
        desc.SourceRect = &src;
        desc.Position = SIMPEngine::TileToWorld(*map, *transform, at) + cell * 0.5f;
        SIMPEngine::Renderer::DrawSprite(desc);
    };

    if (ghostRect)
    {
        glm::ivec2 rlo = Min(m_RectStart, m_Hover);
        glm::ivec2 rhi = Max(m_RectStart, m_Hover);
        if ((rhi.x - rlo.x + 1) * (rhi.y - rlo.y + 1) <= 4096)
            for (int y = rlo.y; y <= rhi.y; ++y)
                for (int x = rlo.x; x <= rhi.x; ++x)
                    ghost({x, y}, tiles.StampAt(x - rlo.x, rhi.y - y));
        return;
    }

    for (int sy = 0; sy < tiles.StampHeight; ++sy)
        for (int sx = 0; sx < tiles.StampWidth; ++sx)
            ghost({m_Hover.x + sx, m_Hover.y - sy}, tiles.StampAt(sx, sy));
}

void TileEditor::DrawScreen(ImDrawList *drawList, const std::function<ImVec2(const glm::vec2 &)> &toScreen) const
{
    using namespace EditorUI;

    TilemapComponent *map = Map();
    TransformComponent *transform = Transform();
    if (!IsActive() || !map || !transform)
        return;

    glm::ivec2 usedMin, usedMax;
    if (SIMPEngine::GetUsedBounds(*map, usedMin, usedMax))
    {
        ImVec2 a = toScreen(SIMPEngine::TileToWorld(*map, *transform, usedMin));
        ImVec2 b = toScreen(SIMPEngine::TileToWorld(*map, *transform, usedMax + glm::ivec2(1)));
        drawList->AddRect(ImMin(a, b), ImMax(a, b), U32(Colors::Accent, 0.45f));
    }

    if (!m_HoverValid && m_Stroke == Stroke::None)
        return;

    const TileEditState &tiles = m_Context->Tiles;
    bool erase = m_Stroke == Stroke::Erase || m_Stroke == Stroke::RectErase ||
                 (m_Stroke == Stroke::None && (tiles.Tool == TileTool::Erase || ImGui::IsMouseDown(ImGuiMouseButton_Right)));
    ImU32 color = erase ? U32(Colors::Error) : U32(Colors::Accent);

    glm::ivec2 lo, hi;
    if (m_Stroke == Stroke::Rect || m_Stroke == Stroke::RectErase)
    {
        lo = Min(m_RectStart, m_Hover);
        hi = Max(m_RectStart, m_Hover);
    }
    else
    {
        bool single = tiles.Tool == TileTool::Fill || tiles.Tool == TileTool::Picker || ImGui::GetIO().KeyAlt;
        int w = single ? 1 : tiles.StampWidth;
        int h = single ? 1 : tiles.StampHeight;
        lo = {m_Hover.x, m_Hover.y - h + 1};
        hi = {m_Hover.x + w - 1, m_Hover.y};
    }

    ImVec2 a = toScreen(SIMPEngine::TileToWorld(*map, *transform, lo));
    ImVec2 b = toScreen(SIMPEngine::TileToWorld(*map, *transform, hi + glm::ivec2(1)));
    ImVec2 mn = ImMin(a, b), mx = ImMax(a, b);
    if (erase)
        drawList->AddRectFilled(mn, mx, U32(Colors::Error, 0.18f));
    else if (m_Stroke == Stroke::Rect)
        drawList->AddRectFilled(mn, mx, U32(Colors::Accent, 0.10f));
    drawList->AddRect(mn, mx, color, 0.0f, 0, 1.5f);

    if (m_Stroke == Stroke::Rect || m_Stroke == Stroke::RectErase)
    {
        char size[32];
        std::snprintf(size, sizeof(size), "%d x %d", hi.x - lo.x + 1, hi.y - lo.y + 1);
        drawList->AddText(ImVec2(mx.x + 6.0f, mn.y), IM_COL32(255, 255, 255, 220), size);
    }
}
