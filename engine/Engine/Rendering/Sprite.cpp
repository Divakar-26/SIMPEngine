#include"Sprite.h"
#include"Renderer.h"

#include"Log.h"

namespace SIMPEngine{

    Sprite::Sprite(std::shared_ptr<Texture> Texture) : m_Texture(Texture), m_HasSourceRect(false) {}
    Sprite::Sprite(std::shared_ptr<Texture> Texture, SDL_FRect srcRect) : m_Texture(Texture), m_SoourceRect(srcRect), m_HasSourceRect(true) {}

    void Sprite::SetTexture(std::shared_ptr<Texture> texture){
        m_Texture = texture;
    }

    void Sprite::SetSourceRect(const SDL_FRect rect){
        m_SoourceRect = rect;
        m_HasSourceRect = true;
    }

    void Sprite::Draw(float x, float y, float width, float height, SDL_Color tint, float rotation){
        if(!m_Texture)
            return;
        
        SDL_Texture * sdlText = m_Texture->GetSDLTexture();
        if(!sdlText)
            return;

        if(m_HasSourceRect){
            Renderer::DrawTexture(sdlText, x, y, width, height, tint, rotation, &m_SoourceRect);
        }
        else{
            CORE_ERROR("No SOURCE RECT");
            Renderer::DrawTexture(sdlText, x, y , width, height, tint, rotation);
        }
    }

}