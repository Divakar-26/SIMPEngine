#include <Engine/Rendering/GameView.h>

#include <Engine/Rendering/GL.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scene/Scene.h>

namespace SIMPEngine
{
    ViewLayout GameView::Render(Scene &scene, int windowWidth, int windowHeight, Framebuffer *target)
    {
        const ViewLayout layout = Display::Compute(windowWidth, windowHeight);
        auto &camera = scene.GetActiveCamera();
        const glm::vec4 &bg = scene.GetBackgroundColor();

        auto bindTarget = [&]()
        {
            if (target)
                Renderer::BeginRenderTarget(*target);
            else
                Renderer::EndRenderTarget();
        };

        if (!layout.Offscreen)
        {
            camera.SetViewportSize(static_cast<float>(layout.WindowWidth), static_cast<float>(layout.WindowHeight));
            bindTarget();
            Renderer::SetClearColor(bg.r, bg.g, bg.b, bg.a);
            Renderer::Clear();
            scene.OnRender();
            Renderer::Flush();
            return layout;
        }

        if (!m_Offscreen)
            m_Offscreen = std::make_unique<Framebuffer>(layout.RenderWidth, layout.RenderHeight);
        else if (m_Offscreen->GetWidth() != layout.RenderWidth || m_Offscreen->GetHeight() != layout.RenderHeight)
            m_Offscreen->Resize(layout.RenderWidth, layout.RenderHeight);

        camera.SetViewportSize(static_cast<float>(layout.RenderWidth), static_cast<float>(layout.RenderHeight));
        Renderer::BeginRenderTarget(*m_Offscreen);
        Renderer::SetClearColor(bg.r, bg.g, bg.b, bg.a);
        Renderer::Clear();
        scene.OnRender();
        Renderer::Flush();

        bindTarget();
        Renderer::SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        Renderer::Clear();

        const bool crisp = layout.Width % layout.RenderWidth == 0 && layout.Height % layout.RenderHeight == 0;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_Offscreen->GetResolveID());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target ? target->GetID() : 0);
        glBlitFramebuffer(0, 0, layout.RenderWidth, layout.RenderHeight,
                          layout.X, layout.Y, layout.X + layout.Width, layout.Y + layout.Height,
                          GL_COLOR_BUFFER_BIT, crisp ? GL_NEAREST : GL_LINEAR);
        bindTarget();
        return layout;
    }
}
