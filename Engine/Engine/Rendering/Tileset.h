#pragma once
#include <Engine/Rendering/Texture.h>
#include <SDL3/SDL_rect.h>
#include <memory>
#include <string>

namespace SIMPEngine
{
    class Tileset
    {
    public:
        Tileset(std::shared_ptr<Texture> tex, int tileWidth, int tileHeight, int margin = 0, int spacing = 0);

        SDL_FRect GetTileRect(int tileID) const;
        bool IsValid(int tileID) const { return tileID >= 0 && tileID < GetTileCount(); }

        int GetColumns() const { return m_Columns; }
        int GetRows() const { return m_Rows; }
        int GetTileCount() const { return m_Columns * m_Rows; }
        int GetTileWidth() const { return m_TileWidth; }
        int GetTileHeight() const { return m_TileHeight; }
        int GetMargin() const { return m_Margin; }
        int GetSpacing() const { return m_Spacing; }
        std::shared_ptr<Texture> GetTexture() const { return m_Texture; }

        const std::string &GetSourcePath() const { return m_SourcePath; }
        void SetSourcePath(const std::string &path) { m_SourcePath = path; }

    private:
        std::shared_ptr<Texture> m_Texture;
        int m_TileWidth;
        int m_TileHeight;
        int m_Margin;
        int m_Spacing;
        int m_Columns;
        int m_Rows;
        std::string m_SourcePath;
    };
}
