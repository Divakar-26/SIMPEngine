// Renderer.h
#pragma once
#include "RenderingAPI.h"

namespace SIMPEngine {

    class Renderer {
    public:
        static void Init(RenderingAPI* api, SDL_Renderer* sdlRenderer);
        static void SetClearColor(float r, float g, float b, float a);
        static void Clear();
        static void DrawQuad(float x, float y, float width, float height, SDL_Color color);
        static void Present();

    private:
        static RenderingAPI* s_RenderingAPI;
    };

}
