#pragma once

#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/Texture.h>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <unordered_map>

struct LightComponent;

namespace SIMPEngine
{
    struct SceneLighting
    {
        bool Enabled = false;
        glm::vec4 Ambient{0.16f, 0.16f, 0.22f, 1.0f};
    };

    class LightingSystem
    {
    public:
        void Render(entt::registry &registry, const SceneLighting &lighting);

        static std::shared_ptr<Texture> GetLightTexture(const LightComponent &light);
        static void ReleaseTextures();

    private:
        std::unordered_map<const void *, std::unique_ptr<Framebuffer>> m_LightMaps;
    };
}
