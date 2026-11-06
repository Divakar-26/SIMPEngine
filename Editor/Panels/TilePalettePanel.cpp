#define IMGUI_DEFINE_MATH_OPERATORS
#include "TilePalettePanel.h"
#include "../EditorCommands.h"
#include "UI/Widgets.h"

#include <Engine/Rendering/Tileset.h>

#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

void TilePalettePanel::OnImGuiRender()
{
    using namespace EditorUI;

    if (m_Context->Tiles.FocusPalette)
    {
        ImGui::SetNextWindowFocus();
        m_Context->Tiles.FocusPalette = false;
    }

    if (ImGuiWindow *browser = ImGui::FindWindowByName("FileSystem###ContentBrowser"); browser && browser->DockId)
        ImGui::SetNextWindowDockID(browser->DockId, ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool visible = ImGui::Begin("Tiles###TilePalette");
    ImGui::PopStyleVar();
    if (!visible)
    {
        ImGui::End();
        return;
    }

    SIMPEngine::Entity entity = m_Context->SelectedEntity;
    auto &registry = m_Context->Scene->GetRegistry();
    TilemapComponent *tilemap = (entity && registry.valid(entity.GetHandle())) ? registry.try_get<TilemapComponent>(entity.GetHandle()) : nullptr;

    if (!tilemap)
    {
        EmptyState("No tilemap selected", "Select an entity with a Tilemap component, or create one.");
        float bw = 140.0f;
        ImGui::SetCursorPos(ImVec2(std::max(8.0f, (ImGui::GetWindowWidth() - bw) * 0.5f), ImGui::GetWindowHeight() * 0.55f));
        if (Button("New Tilemap", ButtonKind::Primary, ImVec2(bw, 0)) && m_Context->IsEditing())
            EditorCommands::CreateTilemap(*m_Context);
        ImGui::End();
        return;
    }

    TileEditState &tiles = m_Context->Tiles;

    BeginToolbar("##palette_toolbar");
    bool active = tiles.Enabled && m_Context->IsEditing();
    ImGui::BeginDisabled(!m_Context->IsEditing());
    if (IconButton("##paint", ICON_LC_PAINTBRUSH, active ? "Stop painting (T)" : "Paint in the viewport (T)", active))
        tiles.Enabled = !tiles.Enabled;
    ImGui::EndDisabled();
    ImGui::SameLine();
    VerticalSeparator();
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    if (tiles.StampWidth == 1 && tiles.StampHeight == 1)
        ImGui::TextDisabled("Tile %d", tiles.StampAt(0, 0));
    else
        ImGui::TextDisabled("Stamp %d x %d", tiles.StampWidth, tiles.StampHeight);

    float sliderW = 110.0f;
    ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 8.0f, ImGui::GetWindowContentRegionMax().x - sliderW - 50.0f));
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Zoom");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(sliderW);
    ImGui::SliderFloat("##zoom", &m_Zoom, 1.0f, 6.0f, "%.1fx");
    EndToolbar();

    if (!tilemap->tileset || !tilemap->tileset->GetTexture())
    {
        EmptyState("No tileset", "Drop an image or a Tiled tileset (.json/.tsj) on the Tileset slot in the Inspector.");
        ImGui::End();
        return;
    }

    DrawPalette(*tilemap);
    ImGui::End();
}

void TilePalettePanel::DrawPalette(TilemapComponent &tilemap)
{
    using namespace EditorUI;

    const SIMPEngine::Tileset &set = *tilemap.tileset;
    auto texture = set.GetTexture();
    TileEditState &tiles = m_Context->Tiles;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Frame);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::BeginChild("##palette", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PopStyleVar();

    if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f)
        m_Zoom = std::clamp(m_Zoom + ImGui::GetIO().MouseWheel * 0.5f, 1.0f, 6.0f);

    float zoom = std::floor(m_Zoom * 2.0f) * 0.5f;
    ImVec2 size(static_cast<float>(texture->GetWidth()) * zoom, static_cast<float>(texture->GetHeight()) * zoom);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##sheet", size);
    bool hovered = ImGui::IsItemHovered();

    ImDrawList *dl = ImGui::GetWindowDrawList();
    DrawCheckerboard(dl, origin, origin + size, 8.0f);
    dl->AddImage(TextureId(texture->GetID()), origin, origin + size, ImVec2(0, 1), ImVec2(1, 0));

    int columns = set.GetColumns();
    int rows = set.GetRows();
    auto tileRect = [&](int col, int row, ImVec2 &mn, ImVec2 &mx)
    {
        SDL_FRect r = set.GetTileRect(row * columns + col);
        mn = origin + ImVec2(r.x, r.y) * zoom;
        mx = mn + ImVec2(r.w, r.h) * zoom;
    };

    ImU32 gridColor = IM_COL32(255, 255, 255, 30);
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < columns; ++col)
        {
            ImVec2 mn, mx;
            tileRect(col, row, mn, mx);
            dl->AddRect(mn, mx + ImVec2(1, 1), gridColor);
        }
    }

    auto cellAt = [&](const ImVec2 &mouse)
    {
        ImVec2 local = (mouse - origin) / zoom;
        float stepX = static_cast<float>(set.GetTileWidth() + set.GetSpacing());
        float stepY = static_cast<float>(set.GetTileHeight() + set.GetSpacing());
        int col = static_cast<int>(std::floor((local.x - static_cast<float>(set.GetMargin())) / stepX));
        int row = static_cast<int>(std::floor((local.y - static_cast<float>(set.GetMargin())) / stepY));
        return glm::ivec2(std::clamp(col, 0, columns - 1), std::clamp(row, 0, rows - 1));
    };

    ImVec2 mouse = ImGui::GetIO().MousePos;
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        m_Selecting = true;
        m_SelectStart = cellAt(mouse);
    }

    glm::ivec2 selLo, selHi;
    bool showSelection = false;
    if (m_Selecting)
    {
        glm::ivec2 current = cellAt(mouse);
        selLo = glm::min(m_SelectStart, current);
        selHi = glm::max(m_SelectStart, current);
        showSelection = true;

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            m_Selecting = false;
            tiles.StampWidth = selHi.x - selLo.x + 1;
            tiles.StampHeight = selHi.y - selLo.y + 1;
            tiles.Stamp.clear();
            for (int row = selLo.y; row <= selHi.y; ++row)
                for (int col = selLo.x; col <= selHi.x; ++col)
                    tiles.Stamp.push_back(row * columns + col);
            if (tiles.Tool == TileTool::Erase || tiles.Tool == TileTool::Picker)
                tiles.Tool = TileTool::Brush;
            if (m_Context->IsEditing())
                tiles.Enabled = true;
        }
    }
    else if (!tiles.Stamp.empty() && tiles.Stamp.front() >= 0)
    {
        int first = tiles.Stamp.front();
        selLo = {first % columns, first / columns};
        selHi = selLo + glm::ivec2(tiles.StampWidth - 1, tiles.StampHeight - 1);
        showSelection = first < set.GetTileCount();
    }

    if (hovered && !m_Selecting)
    {
        glm::ivec2 c = cellAt(mouse);
        ImVec2 mn, mx;
        tileRect(c.x, c.y, mn, mx);
        dl->AddRect(mn, mx, IM_COL32(255, 255, 255, 160));
        ImGui::SetTooltip("Tile %d  (%d, %d)", c.y * columns + c.x, c.x, c.y);
    }

    if (showSelection)
    {
        ImVec2 a, b, c, d;
        tileRect(selLo.x, selLo.y, a, b);
        tileRect(std::min(selHi.x, columns - 1), std::min(selHi.y, rows - 1), c, d);
        dl->AddRectFilled(a, d, U32(Colors::Accent, 0.18f));
        dl->AddRect(a - ImVec2(1, 1), d + ImVec2(1, 1), U32(Colors::Accent), 0.0f, 0, 2.0f);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}
