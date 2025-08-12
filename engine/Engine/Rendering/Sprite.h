#pragma once

#include "Renderer.h"
#include "Texture.h"
#include <memory>


namespace SIMPEngine
{
    class Sprite
    {
    public:
        Sprite(std::shared_ptr<Texture> Texture);
        Sprite(std::shared_ptr<Texture> Texture, SDL_FRect srcRect);

        void SetTexture(std::shared_ptr<Texture> texture);
        void SetSourceRect(const SDL_FRect srcRect);

        void Draw(float x, float y, float width, float height, SDL_Color tint = {255,255,255,255}, float rotation = 0.0f);

    private:
        std::shared_ptr<Texture> m_Texture;
        
        SDL_FRect m_SoourceRect;

        bool m_HasSourceRect;
    };
}