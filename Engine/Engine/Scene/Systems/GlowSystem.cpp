#include "GlowSystem.h"
#include "RenderSystem.h"
#include <Engine/Scene/Component.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Math/EntityUtility.h>

#include <algorithm>

namespace SIMPEngine
{
    void GlowSystem::Render(entt::registry &registry)
    {
        auto view = registry.view<GlowComponent, TransformComponent>();
        float spread = 0.0f;
        m_Visible.clear();
        for (auto entity : view)
        {
            const auto &glow = view.get<GlowComponent>(entity);
            if (!glow.enabled || glow.intensity <= 0.0f)
                continue;
            auto world = DecomposeWorld(view.get<TransformComponent>(entity));
            glm::vec2 reach(600.0f);
            if (!Renderer::IsVisible(world.Position, reach * glm::max(glm::abs(world.Scale), glm::vec2(1.0f))))
                continue;
            m_Visible.push_back(entity);
            spread = std::max(spread, glow.spread);
        }
        if (m_Visible.empty() || spread <= 0.0f)
            return;

        Renderer::Flush();
        RenderingAPI *api = Renderer::GetAPI();
        Framebuffer *target = api->GetRenderTarget();
        glm::vec2 viewport = api->GetViewportSize();
        int width = std::max(1, static_cast<int>(viewport.x) / 2);
        int height = std::max(1, static_cast<int>(viewport.y) / 2);

        auto &emission = m_Emission[target];
        if (!emission)
            emission = std::make_unique<Framebuffer>(width, height, true);
        else
            emission->Resize(width, height);

        BlendMode previousBlend = Renderer::GetBlendMode();
        glm::vec4 previousMultiplier = Renderer::GetColorMultiplier();

        Renderer::BeginRenderTarget(*emission);
        Renderer::SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        Renderer::Clear();
        Renderer::SetBlendMode(BlendMode::Additive);

        for (auto entity : m_Visible)
        {
            const auto &glow = view.get<GlowComponent>(entity);
            Renderer::SetColorMultiplier(glm::vec4(glow.color.r / 255.0f * glow.intensity,
                                                   glow.color.g / 255.0f * glow.intensity,
                                                   glow.color.b / 255.0f * glow.intensity,
                                                   glow.color.a / 255.0f));
            RenderSystem::DrawEntity(registry, entity);
        }

        Renderer::Flush();
        Renderer::SetColorMultiplier(previousMultiplier);
        Renderer::SetBlendMode(previousBlend);
        if (target)
            Renderer::BeginRenderTarget(*target);
        else
            Renderer::EndRenderTarget();

        m_Blur.ApplyGlow(emission->GetColorAttachment(), width, height, spread);
    }
}
