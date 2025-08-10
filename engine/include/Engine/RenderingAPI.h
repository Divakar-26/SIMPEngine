#pragma once
#include<SDL3/SDL.h>

namespace SIMPEngine{
    class RenderingAPI{
        public:
            virtual ~RenderingAPI();

            virtual void Init(SDL_Renderer * sdlRenderer) = 0;
            virtual void SetClearColor(float r, float g, float b, float a) = 0;
            virtual void Clear() = 0;
            virtual void DrawQuad(float x, float y, float width, float height, SDL_Color color) = 0;
            virtual void Present() = 0;
    };
}