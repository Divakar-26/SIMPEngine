#include "Tileset.h"

namespace SIMPEngine
{
    Tileset::Tileset(std::shared_ptr<Texture> tex, int tileWidth, int tileHeight) : m_Texture(tex), m_TileWidth(tileWidth), m_TileHeight(tileHeight)
    {
        m_Columns = tex->GetWidth() / tileWidth;
    }

    SDL_FRect Tileset::GetTileRect(int tileID) const
    {
        int col = tileID % m_Columns;
        int row = tileID / m_Columns;

        return SDL_FRect{
            (float)(col * m_TileWidth),
            (float)(col * m_TileHeight),
            (float)m_TileWidth,
            (float)m_TileHeight
        };
    }
}