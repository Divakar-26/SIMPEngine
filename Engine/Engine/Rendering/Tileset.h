#pragma once
#include <Engine/Rendering/Texture.h>
#include <SDL3/SDL_rect.h>
#include <memory>

namespace SIMPEngine
{
    class Tileset
    {
    public:
        Tileset(std::shared_ptr<Texture> tex, int tileWidth, int tileHeight);

        SDL_FRect GetTileRect(int tileID) const;

        int GetCloumns() const { return m_Columns; }

        std::shared_ptr<Texture> GetTexture() const { return m_Texture; }

    private:
        std::shared_ptr<Texture> m_Texture;
        int m_TileWidth;
        int m_TileHeight;
        int m_Columns;
    };
}