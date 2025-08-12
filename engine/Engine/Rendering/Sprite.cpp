#include "Sprite.h"
#include "Renderer.h"
#include "Log.h"

namespace SIMPEngine
{
    Sprite::Sprite(std::shared_ptr<Texture> texture)
        : m_Texture(std::move(texture)), m_HasSourceRect(false) {}

    Sprite::Sprite(std::shared_ptr<Texture> texture,const SDL_FRect& srcRect)
        : m_Texture(std::move(texture)), m_SourceRect(srcRect), m_HasSourceRect(true) {}

    void Sprite::SetTexture(std::shared_ptr<Texture> texture)
    {
        m_Texture = std::move(texture);
    }

    void Sprite::SetSourceRect(const SDL_FRect& srcRect)
    {
        m_SourceRect = srcRect;
        m_HasSourceRect = true;
    }

    void Sprite::ClearSourceRect()
    {
        m_HasSourceRect = false;
        m_SourceRect = {0, 0, 0, 0};
    }

    void Sprite::Draw(float x, float y, float width, float height,
                      SDL_Color tint, float rotation) const
    {
        if (!m_Texture)
        {
            CORE_ERROR("Sprite has no texture!");
            return;
        }

        SDL_Texture* sdlTex = m_Texture->GetSDLTexture();
        if (!sdlTex)
        {
            CORE_ERROR("Texture is invalid (SDL_Texture is null)!");
            return;
        }

        if (m_HasSourceRect)
        {
            Renderer::DrawTexture(sdlTex, x, y, width, height, tint, rotation, &m_SourceRect);
        }
        else
        {
            Renderer::DrawTexture(sdlTex, x, y, width, height, tint, rotation);
        }
    }
}
