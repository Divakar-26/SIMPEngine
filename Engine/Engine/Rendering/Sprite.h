#pragma once
#include <Engine/Rendering/Texture.h>

#include <SDL3/SDL.h>

#include <memory>

namespace SIMPEngine
{
    class Sprite
    {
    public:
        Sprite() = default;
        explicit Sprite(std::shared_ptr<Texture> texture);
        Sprite(std::shared_ptr<Texture> texture, const SDL_FRect& srcRect);

        void SetTexture(std::shared_ptr<Texture> texture);
        void SetSourceRect(const SDL_FRect& srcRect);
        void ClearSourceRect();

        void Draw(float x, float y, float width, float height,
                  SDL_Color tint = {255, 255, 255, 255}, float rotation = 0.0f) const;

        bool HasSourceRect() const { return m_HasSourceRect; }

    private:
        std::shared_ptr<Texture> m_Texture = nullptr;
        SDL_FRect m_SourceRect{};
        bool m_HasSourceRect = false;
    };
}
