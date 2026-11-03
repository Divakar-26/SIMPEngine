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
            static_cast<float>(col * m_TileWidth),
            static_cast<float>(row * m_TileHeight),
            static_cast<float>(m_TileWidth),
            static_cast<float>(m_TileHeight)
        };
    }
}
