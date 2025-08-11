#include"Texture.h"
#include<SDL3_image/SDL_image.h>
#include<iostream>

#include"Log.h"

namespace SIMPEngine{

    Texture::Texture(): m_Texture(nullptr), m_Width(0), m_Height(0){}

    Texture::~Texture(){
        Destroy();
    }

    bool Texture::LoadFromFile(SDL_Renderer * renderer, const char * path){
        Destroy();

        SDL_Surface * surface = IMG_Load(path);
        if(!surface){
            CORE_ERROR("Surface Could not be made! {}", SDL_GetError());
            return false;
        }

        m_Width = surface->w;
        m_Height = surface->h;
        m_Texture = SDL_CreateTextureFromSurface(renderer, surface);
        if(!m_Texture){
            CORE_ERROR("Texture coudl not be made using Surface");
            return false;
        }

        SDL_DestroySurface(surface);

        SDL_SetTextureBlendMode(m_Texture, SDL_BLENDMODE_BLEND);
        return true;
    }

    void Texture::Destroy(){
        if(m_Texture){

            SDL_DestroyTexture(m_Texture);
            m_Texture = nullptr;
            m_Width = m_Height = 0;
        }

    }

}

