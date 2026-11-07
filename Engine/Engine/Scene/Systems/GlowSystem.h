#pragma once

#include <Engine/Rendering/Bloom.h>
#include <Engine/Rendering/Framebuffer.h>

#include <entt/entt.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace SIMPEngine
{
    class GlowSystem
    {
    public:
        void Render(entt::registry &registry);

    private:
        std::unordered_map<const void *, std::unique_ptr<Framebuffer>> m_Emission;
        BloomRenderer m_Blur;
        std::vector<entt::entity> m_Visible;
    };
}
