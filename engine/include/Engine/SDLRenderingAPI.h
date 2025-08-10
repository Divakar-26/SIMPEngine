#pragma once
#include"RenderingAPI.h"

namespace SIMPEngine{
    class SDLRenderingAPI : public RenderingAPI{
        public:
            SDLRenderingAPI();
            virtual ~SDLRenderingAPI() = default;
            
            void Init(SDL_Renderer * sdlRenderer) override;
            void SetClearColor(float r, float g, float b, float a) override;
            void Clear() override;
            void DrawQuad(float x, float y, float width, float height, SDL_Color color) override;
            void Present() override;
        
        private:
            SDL_Renderer * m_Renderer = nullptr;
            SDL_Color m_ClearColor {0,0,0,255};

    };
}