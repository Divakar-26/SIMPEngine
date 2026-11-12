#pragma once

#include <Engine/Core/Display.h>
#include <Engine/Rendering/Framebuffer.h>

#include <memory>

namespace SIMPEngine
{
    class Scene;

    class GameView
    {
    public:
        ViewLayout Render(Scene &scene, int windowWidth, int windowHeight, Framebuffer *target);

    private:
        std::unique_ptr<Framebuffer> m_Offscreen;
    };
}
