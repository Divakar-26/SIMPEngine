#include "LightingSystem.h"
#include <Engine/Scene/Component.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Rendering/Renderer.h>

#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace SIMPEngine
{
    namespace
    {
        constexpr int TextureSize = 128;
        std::unordered_map<uint64_t, std::shared_ptr<Texture>> s_Textures;

        uint64_t TextureKey(const LightComponent &light)
        {
            uint64_t falloff = static_cast<uint64_t>(std::clamp(static_cast<int>(std::lround(light.falloff * 10.0f)), 1, 100));
            if (light.type == LightType::Point)
                return falloff;
            uint64_t outer = static_cast<uint64_t>(std::clamp(static_cast<int>(std::lround(light.outerAngle)), 1, 180));
            uint64_t inner = static_cast<uint64_t>(std::clamp(static_cast<int>(std::lround(light.innerAngle)), 0, static_cast<int>(outer)));
            return (1ull << 40) | (falloff << 20) | (inner << 10) | outer;
        }

        float FlickerNoise(float t, float speed, float seed)
        {
            float n = 0.5f + 0.25f * std::sin(t * speed + seed) + 0.15f * std::sin(t * speed * 2.3f + seed * 1.7f) +
                      0.1f * std::sin(t * speed * 5.1f + seed * 0.3f);
            return std::clamp(n, 0.0f, 1.0f);
        }
    }

    std::shared_ptr<Texture> LightingSystem::GetLightTexture(const LightComponent &light)
    {
        uint64_t key = TextureKey(light);
        if (auto it = s_Textures.find(key); it != s_Textures.end())
            return it->second;

        if (s_Textures.size() > 96)
            s_Textures.clear();

        float falloff = static_cast<float>(key & 0xFFFFF) / 10.0f;
        bool spot = light.type == LightType::Spot;
        float outer = 0.0f, inner = 0.0f;
        if (spot)
        {
            falloff = static_cast<float>((key >> 20) & 0xFFFFF) / 10.0f;
            inner = static_cast<float>((key >> 10) & 0x3FF);
            outer = static_cast<float>(key & 0x3FF);
        }

        std::vector<unsigned char> pixels(TextureSize * TextureSize * 4);
        for (int y = 0; y < TextureSize; ++y)
        {
            for (int x = 0; x < TextureSize; ++x)
            {
                float nx = (static_cast<float>(x) + 0.5f) / TextureSize * 2.0f - 1.0f;
                float ny = (static_cast<float>(y) + 0.5f) / TextureSize * 2.0f - 1.0f;
                float r = std::sqrt(nx * nx + ny * ny);
                float a = r >= 1.0f ? 0.0f : std::pow(1.0f - r, falloff);
                if (spot && a > 0.0f)
                {
                    float angle = std::abs(glm::degrees(std::atan2(ny, nx)));
                    float edge = outer > inner ? glm::smoothstep(inner, outer, angle) : (angle > outer ? 1.0f : 0.0f);
                    a *= 1.0f - edge;
                }
                unsigned char *px = &pixels[static_cast<size_t>((y * TextureSize + x) * 4)];
                px[0] = px[1] = px[2] = 255;
                px[3] = static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
            }
        }

        auto texture = std::make_shared<Texture>();
        texture->UploadToGPU(pixels.data(), TextureSize, TextureSize, 4);
        s_Textures[key] = texture;
        return texture;
    }

    void LightingSystem::ReleaseTextures()
    {
        s_Textures.clear();
    }

    void LightingSystem::Render(entt::registry &registry, const SceneLighting &lighting)
    {
        RenderingAPI *api = Renderer::GetAPI();
        Framebuffer *target = api->GetRenderTarget();
        glm::vec2 viewport = api->GetViewportSize();
        int width = std::max(1, static_cast<int>(viewport.x) / 2);
        int height = std::max(1, static_cast<int>(viewport.y) / 2);

        auto &lightMap = m_LightMaps[target];
        if (!lightMap)
            lightMap = std::make_unique<Framebuffer>(width, height, true);
        else
            lightMap->Resize(width, height);

        glm::mat4 inverse = glm::inverse(api->GetProjectionMatrix() * Renderer::GetViewMatrix());
        BlendMode previous = Renderer::GetBlendMode();

        Renderer::BeginRenderTarget(*lightMap);
        Renderer::SetClearColor(lighting.Ambient.r, lighting.Ambient.g, lighting.Ambient.b, 1.0f);
        Renderer::Clear();
        Renderer::SetBlendMode(BlendMode::Additive);

        float time = static_cast<float>(SDL_GetTicks()) / 1000.0f;
        auto view = registry.view<LightComponent, TransformComponent>();
        for (auto entity : view)
        {
            const auto &light = view.get<LightComponent>(entity);
            if (!light.enabled || light.intensity <= 0.0f || light.radius <= 0.0f)
                continue;

            float intensity = light.intensity;
            if (light.flicker > 0.0f)
            {
                float seed = static_cast<float>(entt::to_integral(entity)) * 12.9898f;
                intensity *= 1.0f - std::clamp(light.flicker, 0.0f, 1.0f) * FlickerNoise(time, light.flickerSpeed, seed);
            }

            auto world = DecomposeWorld(view.get<TransformComponent>(entity));
            float scale = (std::abs(world.Scale.x) + std::abs(world.Scale.y)) * 0.5f;
            if (!Renderer::IsVisible(world.Position, glm::vec2(light.radius * scale)))
                continue;
            glm::vec4 color(light.color.r / 255.0f * intensity, light.color.g / 255.0f * intensity, light.color.b / 255.0f * intensity, 1.0f);
            bool spot = light.type == LightType::Spot;
            Renderer::DrawLight(world.Position, light.radius * scale, world.Rotation, color, light.falloff,
                                spot ? std::min(light.innerAngle, light.outerAngle) : 0.0f, spot ? std::max(light.outerAngle, 0.5f) : 0.0f);
        }

        Renderer::Flush();
        if (target)
            Renderer::BeginRenderTarget(*target);
        else
            Renderer::EndRenderTarget();

        glm::vec4 c = inverse * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 rx = inverse * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 ry = inverse * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        glm::vec2 center(c.x, c.y);
        glm::vec2 axisX = glm::vec2(rx.x, rx.y) - center;
        glm::vec2 axisY = glm::vec2(ry.x, ry.y) - center;

        Primitive composite;
        composite.Center = center;
        composite.HalfSize = {glm::length(axisX), glm::length(axisY)};
        composite.Rotation = std::atan2(axisX.y, axisX.x);
        composite.Texture = lightMap->GetColorAttachment();
        composite.Mode = PrimitiveMode::Sprite;
        composite.Blend = BlendMode::Multiply;
        composite.Color = glm::vec4(1.0f);
        api->Submit(composite);
        Renderer::Flush();

        Renderer::SetBlendMode(previous);
    }
}
