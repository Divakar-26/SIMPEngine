#include "Tileset.h"

#include <algorithm>

namespace SIMPEngine
{
    Tileset::Tileset(std::shared_ptr<Texture> tex, int tileWidth, int tileHeight, int margin, int spacing)
        : m_Texture(std::move(tex)),
          m_TileWidth(std::max(1, tileWidth)),
          m_TileHeight(std::max(1, tileHeight)),
          m_Margin(std::max(0, margin)),
          m_Spacing(std::max(0, spacing)),
          m_Columns(1),
          m_Rows(1)
    {
        if (m_Texture)
        {
            m_Columns = std::max(1, (m_Texture->GetWidth() - 2 * m_Margin + m_Spacing) / (m_TileWidth + m_Spacing));
            m_Rows = std::max(1, (m_Texture->GetHeight() - 2 * m_Margin + m_Spacing) / (m_TileHeight + m_Spacing));
        }
    }

    SDL_FRect Tileset::GetTileRect(int tileID) const
    {
        int col = tileID % m_Columns;
        int row = tileID / m_Columns;
        return SDL_FRect{
            static_cast<float>(m_Margin + col * (m_TileWidth + m_Spacing)),
            static_cast<float>(m_Margin + row * (m_TileHeight + m_Spacing)),
            static_cast<float>(m_TileWidth),
            static_cast<float>(m_TileHeight)};
    }
}
