#include "Builder.h"
#include "Scripts.h"
#include "Levels.h"

#include <Engine/Particles/ParticlePresets.h>
#include <Engine/Rendering/Animation.h>

#include <cstdio>

namespace Mossveil
{
    namespace
    {
        const char *TopSlabs[] = {"platforms_07", "platforms_06", "platforms_01", "platforms_04", "tiles_03"};
        const char *SidePillars[] = {"tiles_02", "platforms_02"};
        const char *SmallPlatforms[] = {"platforms_01", "platforms_04", "tiles_03"};
        const char *WidePlatforms[] = {"platforms_06", "platforms_07"};
        const char *Bushes[] = {"hills_00", "hills_02", "hills_04", "hills_06", "decor_25", "decor_30"};
        const char *Sprigs[] = {"decor_10", "decor_11", "decor_12", "decor_22", "decor_23", "decor_24", "decor_26", "decor_27", "decor_28", "decor_29", "decor_21"};
        const char *Rocks[] = {"decor_00", "decor_01", "decor_02"};
        const char *Hangers[] = {"hanging_00", "hanging_01", "hanging_02", "hanging_03", "hanging_06", "hanging_07"};
        const char *Vines[] = {"decor_09", "decor_18", "decor_19", "decor_20"};
        const char *Silhouettes[] = {"background_00", "background_01", "background_02", "background_03", "background_04", "background_05", "hills_01", "hills_05"};

        struct ThornPiece
        {
            const char *Name;
            float Width;
        };
        const ThornPiece Thorns[] = {{"decor_03", 370}, {"decor_04", 308}, {"decor_05", 230}, {"decor_06", 116}, {"decor_07", 154}, {"decor_08", 186}};

        const SheetInfo *GroundPlants[] = {&Sheets::PlantTall, &Sheets::PlantCurl, &Sheets::PlantTuft, &Sheets::PlantBulb, &Sheets::PlantSway,
                                           &Sheets::PlantFern, &Sheets::PlantReed, &Sheets::PlantWind};

        SDL_Color Scale(SDL_Color c, float k)
        {
            return {static_cast<Uint8>(std::clamp(c.r * k, 0.0f, 255.0f)), static_cast<Uint8>(std::clamp(c.g * k, 0.0f, 255.0f)),
                    static_cast<Uint8>(std::clamp(c.b * k, 0.0f, 255.0f)), c.a};
        }

        glm::vec4 ToVec4(SDL_Color c, float alpha)
        {
            return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, alpha};
        }
    }

    Builder::Builder(SIMPEngine::Scene &scene, uint32_t seed)
        : m_Scene(scene), m_Rng(seed)
    {
    }

    float Builder::Random()
    {
        return std::uniform_real_distribution<float>(0.0f, 1.0f)(m_Rng);
    }

    float Builder::Range(float a, float b)
    {
        return a + (b - a) * Random();
    }

    int Builder::Pick(int count)
    {
        return std::min(count - 1, static_cast<int>(Random() * static_cast<float>(count)));
    }

    entt::entity Builder::Group(const char *name)
    {
        for (const auto &[groupName, handle] : m_Groups)
            if (groupName == name)
                return handle;
        auto entity = m_Scene.CreateEntity(name);
        m_Groups.push_back({name, entity.GetHandle()});
        return entity.GetHandle();
    }

    SIMPEngine::Entity Builder::Make(const char *name, glm::vec2 position, float z, entt::entity group)
    {
        auto entity = m_Scene.CreateEntity(name);
        auto &transform = entity.GetComponent<TransformComponent>();
        transform.position = position;
        transform.zIndex = z;
        if (group != entt::null)
            m_Scene.SetParent(entity.GetHandle(), group);
        return entity;
    }

    SIMPEngine::Entity Builder::Emitter(const char *name, glm::vec2 at, const SIMPEngine::ParticleEffect &effect, float z, entt::entity group)
    {
        auto entity = Make(name, at, z, group);
        auto &emitter = entity.AddComponent<ParticleEmitterComponent>();
        emitter.effect = effect;
        emitter.state.Rng = static_cast<uint32_t>(Random() * 4000000000.0f) | 1u;
        return entity;
    }

    void Builder::Atmosphere(const Palette &palette, float bloomThreshold, float bloomIntensity)
    {
        m_Palette = palette;
        m_Scene.SetBackgroundColor({palette.Background, 1.0f});
        auto &lighting = m_Scene.GetLighting();
        lighting.Enabled = true;
        lighting.Ambient = {palette.Ambient, 1.0f};
        auto &bloom = m_Scene.GetBloom();
        bloom.Enabled = true;
        bloom.Threshold = bloomThreshold;
        bloom.Knee = 0.35f;
        bloom.Intensity = bloomIntensity;
        bloom.Radius = 1.2f;
    }

    void Builder::MossRect(glm::vec2 min, glm::vec2 max, uint8_t edges, const char *tag)
    {
        auto collider = Make(tag, min, 0.0f, Group("Colliders"));
        auto &collision = collider.AddComponent<CollisionComponent>();
        collision.width = max.x - min.x;
        collision.height = max.y - min.y;

        entt::entity terrain = Group("Terrain");
        glm::vec2 fillMin = min, fillMax = max;
        if (edges & EdgeTop)
            fillMax.y -= 18.0f;
        if (edges & EdgeBottom)
            fillMin.y += 18.0f;
        if (edges & EdgeLeft)
            fillMin.x += 14.0f;
        if (edges & EdgeRight)
            fillMax.x -= 14.0f;
        auto fill = Make("Rock", (fillMin + fillMax) * 0.5f, 3.0f, terrain);
        auto &render = fill.AddComponent<RenderComponent>();
        render.width = fillMax.x - fillMin.x;
        render.height = fillMax.y - fillMin.y;
        render.color = {14, 24, 24, 255};

        auto horizontal = [&](float y, bool flipY)
        {
            float width = max.x - min.x;
            float x = min.x;
            while (x < max.x)
            {
                auto texture = Tex(PiecePath(TopSlabs[Pick(5)]));
                float scale = Range(0.5f, 0.6f);
                float w = texture->GetWidth() * scale;
                float h = texture->GetHeight() * scale;
                if (width < w * 0.8f)
                {
                    scale *= width / (w * 0.8f);
                    w = texture->GetWidth() * scale;
                    h = texture->GetHeight() * std::max(scale, 0.35f);
                }
                float cx = std::min(x + w * 0.35f, max.x - w * 0.35f);
                if (width < w * 0.8f)
                    cx = (min.x + max.x) * 0.5f;
                auto slab = Make("Moss", {cx, y}, 2.0f, terrain);
                auto &sprite = slab.AddComponent<SpriteComponent>();
                sprite.texture = texture;
                sprite.width = w;
                sprite.height = h;
                sprite.flipX = Random() < 0.5f;
                sprite.flipY = flipY;
                sprite.tint = Scale({255, 255, 255, 255}, Range(0.85f, 1.0f));
                if (width < w * 0.8f)
                    break;
                x += w * Range(0.5f, 0.62f);
            }
        };

        auto vertical = [&](float x, float y0, float y1)
        {
            float y = y1;
            while (y > y0)
            {
                auto texture = Tex(PiecePath(SidePillars[Pick(2)]));
                float scale = Range(0.5f, 0.58f);
                float w = texture->GetWidth() * scale;
                float h = texture->GetHeight() * scale;
                auto pillar = Make("Moss", {x, std::max(y - h * 0.4f, y0 + h * 0.3f)}, 2.0f, terrain);
                auto &sprite = pillar.AddComponent<SpriteComponent>();
                sprite.texture = texture;
                sprite.width = w;
                sprite.height = h;
                sprite.flipY = Random() < 0.5f;
                sprite.tint = Scale({255, 255, 255, 255}, Range(0.8f, 0.95f));
                y -= h * 0.62f;
            }
        };

        if (edges & EdgeTop)
            horizontal(max.y - 4.0f, false);
        if (edges & EdgeBottom)
            horizontal(min.y + 4.0f, true);

        float sideTop = (edges & EdgeTop) ? max.y - 20.0f : std::min(max.y, min.y + 1100.0f);
        float sideBottom = (edges & EdgeBottom) ? min.y + 20.0f : std::max(min.y, max.y - 1100.0f);
        if ((edges & EdgeTop) && (edges & EdgeBottom))
        {
            sideTop = max.y - 20.0f;
            sideBottom = min.y + 20.0f;
        }
        if (edges & EdgeLeft)
            vertical(min.x + 6.0f, sideBottom, sideTop);
        if (edges & EdgeRight)
            vertical(max.x - 6.0f, sideBottom, sideTop);
    }

    void Builder::Ground(float x0, float x1, float top, float bottom, uint8_t sides)
    {
        MossRect({x0, bottom}, {x1, top}, static_cast<uint8_t>(EdgeTop | sides));
    }

    void Builder::Ceiling(float x0, float x1, float bottom, float top, uint8_t sides)
    {
        MossRect({x0, bottom}, {x1, top}, static_cast<uint8_t>(EdgeBottom | sides));
    }

    void Builder::Wall(float x0, float x1, float y0, float y1)
    {
        MossRect({x0, y0}, {x1, y1}, EdgeAll);
    }

    void Builder::Platform(float cx, float top, float width)
    {
        const char *name = width <= 330.0f ? SmallPlatforms[Pick(3)] : WidePlatforms[Pick(2)];
        auto texture = Tex(PiecePath(name));
        float scale = width / static_cast<float>(texture->GetWidth());
        float height = texture->GetHeight() * std::min(scale, 1.1f);

        auto platform = Make("Platform", {cx, top - height * 0.2f}, 2.5f, Group("Terrain"));
        auto &sprite = platform.AddComponent<SpriteComponent>();
        sprite.texture = texture;
        sprite.width = width;
        sprite.height = height;
        sprite.flipX = Random() < 0.5f;

        auto collider = Make("OneWay", {cx - width * 0.44f, top - 28.0f}, 0.0f, Group("Colliders"));
        auto &collision = collider.AddComponent<CollisionComponent>();
        collision.width = width * 0.88f;
        collision.height = 28.0f;

        if (Random() < 0.6f)
            Hanging(Hangers[Pick(6)], cx + Range(-width * 0.3f, width * 0.3f), top - height * 0.45f, Range(0.35f, 0.5f), 2.3f, Scale({255, 255, 255, 255}, 0.8f));
    }

    void Builder::Thorns(float cx, float surface, float width)
    {
        const ThornPiece *best = &Mossveil::Thorns[0];
        for (const auto &piece : Mossveil::Thorns)
            if (std::abs(piece.Width * 0.7f - width) < std::abs(best->Width * 0.7f - width))
                best = &piece;

        auto texture = Tex(PiecePath(best->Name));
        float scale = width / (texture->GetWidth() * 0.85f);
        float w = texture->GetWidth() * scale;
        float h = texture->GetHeight() * scale;

        auto thorn = Make("Thorns", {cx, surface + h * 0.42f}, 2.2f, Group("Hazards"));
        auto &sprite = thorn.AddComponent<SpriteComponent>();
        sprite.texture = texture;
        sprite.width = w;
        sprite.height = h;
        sprite.flipX = Random() < 0.5f;

        auto collider = Make("Hazard", {cx - w * 0.36f, surface}, 0.0f, Group("Colliders"));
        auto &collision = collider.AddComponent<CollisionComponent>();
        collision.width = w * 0.72f;
        collision.height = h * 0.55f;
    }

    void Builder::Plant(const SheetInfo &sheet, float x, float ground, float height, float z, SDL_Color tint)
    {
        float width = height * static_cast<float>(sheet.FrameWidth) / static_cast<float>(sheet.FrameHeight);
        auto plant = Make("Plant", {x, ground + height * 0.46f}, z, Group("Flora"));
        auto &sprite = plant.AddComponent<SpriteComponent>();
        sprite.width = width;
        sprite.height = height;
        sprite.tint = tint;
        auto &anim = plant.AddComponent<AnimatedSpriteComponent>();
        anim.animation = std::make_unique<SIMPEngine::Animation>(Tex(SheetPath(sheet)), sheet.FrameWidth, sheet.FrameHeight, sheet.Frames, Range(0.045f, 0.06f), true);
        anim.animation->SetPlaybackState(Pick(sheet.Frames), 0.0f, false);
    }

    void Builder::GlowPlant(const SheetInfo &sheet, float x, float ground, float height, SDL_Color light, float z)
    {
        Plant(sheet, x, ground, height, z);
        auto lamp = Make("Glowcap", {x, ground + height * 0.7f}, z, Group("Lights"));
        auto &l = lamp.AddComponent<LightComponent>();
        l.color = light;
        l.radius = height * 2.4f;
        l.intensity = 0.95f;
        l.falloff = 1.2f;
        l.flicker = 0.12f;
        l.flickerSpeed = 2.5f;

        SIMPEngine::ParticleEffect motes;
        motes.Name = "Glowcap Motes";
        motes.Emitter = SIMPEngine::EmitterShape::Circle;
        motes.Radius = height * 0.35f;
        motes.Rate = 3.0f;
        motes.MaxParticles = 30;
        motes.Prewarm = true;
        motes.Lifetime = {2.0f, 3.5f};
        motes.Direction = 90.0f;
        motes.Spread = 40.0f;
        motes.Speed = {8.0f, 22.0f};
        motes.Turbulence = 10.0f;
        motes.Size = {4.0f, 7.0f};
        motes.EndSize = 0.2f;
        motes.StartColor = ToVec4(light, 0.9f);
        motes.EndColor = ToVec4(light, 0.0f);
        motes.FadeIn = 0.2f;
        motes.Shape = SIMPEngine::ParticleShape::Soft;
        motes.Blend = SIMPEngine::BlendMode::Additive;
        motes.Emissive = true;
        Emitter("Glowcap Motes", {x, ground + height * 0.8f}, motes, z + 0.1f, Group("Effects"));
    }

    void Builder::Decor(const std::string &piece, float x, float ground, float scale, float z, SDL_Color tint, bool flip)
    {
        auto texture = Tex(PiecePath(piece));
        float w = texture->GetWidth() * scale;
        float h = texture->GetHeight() * scale;
        auto decor = Make("Decor", {x, ground + h * 0.44f}, z, Group("Flora"));
        auto &sprite = decor.AddComponent<SpriteComponent>();
        sprite.texture = texture;
        sprite.width = w;
        sprite.height = h;
        sprite.tint = tint;
        sprite.flipX = flip;
    }

    void Builder::Hanging(const std::string &piece, float x, float ceiling, float scale, float z, SDL_Color tint)
    {
        auto texture = Tex(PiecePath(piece));
        float w = texture->GetWidth() * scale;
        float h = texture->GetHeight() * scale;
        auto decor = Make("Hanging", {x, ceiling - h * 0.46f}, z, Group("Flora"));
        auto &sprite = decor.AddComponent<SpriteComponent>();
        sprite.texture = texture;
        sprite.width = w;
        sprite.height = h;
        sprite.tint = tint;
        sprite.flipX = Random() < 0.5f;
    }

    void Builder::Foliage(float x0, float x1, float ground, float density)
    {
        float x = x0 + Range(20.0f, 80.0f);
        while (x < x1 - 30.0f)
        {
            float roll = Random();
            bool front = Random() < 0.18f;
            float z = front ? 6.0f : Range(1.0f, 1.8f);
            SDL_Color tint = front ? SDL_Color{70, 90, 90, 255} : Scale({255, 255, 255, 255}, Range(0.75f, 1.0f));
            if (roll < 0.45f)
                Plant(*GroundPlants[Pick(8)], x, ground - 6.0f, Range(90.0f, 170.0f) * (front ? 1.25f : 1.0f), z, tint);
            else if (roll < 0.7f)
                Decor(Sprigs[Pick(11)], x, ground - 6.0f, Range(0.6f, 1.0f), z, tint, Random() < 0.5f);
            else if (roll < 0.9f)
                Decor(Bushes[Pick(6)], x, ground - 12.0f, Range(0.5f, 0.85f), z, tint, Random() < 0.5f);
            else if (x < x1 - 160.0f && !front)
                Decor(Rocks[Pick(3)], x, ground - 14.0f, Range(0.4f, 0.6f), Range(1.0f, 1.5f), Scale({255, 255, 255, 255}, 0.85f), Random() < 0.5f);
            x += Range(70.0f, 190.0f) / std::max(0.2f, density);
        }
    }

    void Builder::HangingFoliage(float x0, float x1, float ceiling, float density)
    {
        float x = x0 + Range(40.0f, 120.0f);
        while (x < x1 - 40.0f)
        {
            if (Random() < 0.65f)
                Hanging(Hangers[Pick(6)], x, ceiling + 10.0f, Range(0.5f, 0.85f), Range(1.0f, 2.4f), Scale({255, 255, 255, 255}, Range(0.7f, 1.0f)));
            else
                Hanging(Vines[Pick(4)], x, ceiling + 20.0f, Range(0.5f, 0.9f), Range(1.0f, 2.4f), Scale({255, 255, 255, 255}, Range(0.6f, 0.9f)));
            x += Range(120.0f, 280.0f) / std::max(0.2f, density);
        }
    }

    void Builder::Background(float x0, float x1, float baseY, float factor, float z, SDL_Color tint, float spacing, float minScale, float maxScale)
    {
        char name[48];
        std::snprintf(name, sizeof(name), "Parallax %.2f", factor);
        entt::entity group = Group(name);
        auto groupEntity = SIMPEngine::Entity(group, &m_Scene);
        if (!groupEntity.HasComponent<ParallaxComponent>())
            groupEntity.AddComponent<ParallaxComponent>().factor = {factor, 1.0f};

        float start = x0 * factor - 1400.0f;
        float end = x1 * factor + 1400.0f;
        for (float x = start; x < end; x += spacing * Range(0.6f, 1.2f))
        {
            auto texture = Tex(PiecePath(Silhouettes[Pick(8)]));
            float scale = Range(minScale, maxScale);
            float w = texture->GetWidth() * scale;
            float h = texture->GetHeight() * scale;
            auto shape = Make("Silhouette", {x, baseY + h * 0.5f + Range(-80.0f, 40.0f)}, z, group);
            auto &sprite = shape.AddComponent<SpriteComponent>();
            sprite.texture = texture;
            sprite.width = w;
            sprite.height = h;
            sprite.tint = Scale(tint, Range(0.85f, 1.1f));
            sprite.flipX = Random() < 0.5f;
        }
    }

    void Builder::Fog(float x0, float x1, float y, float height, float factor, float z, SDL_Color color)
    {
        (void)factor;
        SIMPEngine::ParticleEffect fog;
        fog.Name = "Fog Bank";
        fog.Emitter = SIMPEngine::EmitterShape::Box;
        fog.BoxSize = {x1 - x0 + 1600.0f, height};
        fog.Rate = (x1 - x0) / 9000.0f;
        fog.MaxParticles = std::max(6, static_cast<int>((x1 - x0) / 1100.0f));
        fog.Prewarm = true;
        fog.Lifetime = {16.0f, 24.0f};
        fog.Direction = 0.0f;
        fog.Spread = 20.0f;
        fog.Speed = {6.0f, 16.0f};
        fog.Size = {700.0f, 1000.0f};
        fog.EndSize = 1.3f;
        fog.StartRotation = {0.0f, 360.0f};
        fog.AngularVelocity = {-3.0f, 3.0f};
        fog.StartColor = ToVec4(color, std::min(1.0f, color.a / 255.0f * 2.2f));
        fog.EndColor = ToVec4(color, 0.0f);
        fog.FadeIn = 0.35f;
        fog.Shape = SIMPEngine::ParticleShape::Smoke;
        Emitter("Fog Bank", {(x0 + x1) * 0.5f, y + height * 0.5f}, fog, z, Group("Effects"));
    }

    void Builder::Light(glm::vec2 at, SDL_Color color, float radius, float intensity, float falloff, float flicker)
    {
        auto entity = Make("Light", at, 0.0f, Group("Lights"));
        auto &light = entity.AddComponent<LightComponent>();
        light.color = color;
        light.radius = radius;
        light.intensity = intensity;
        light.falloff = falloff;
        light.flicker = flicker;
        light.flickerSpeed = 3.0f;
    }

    void Builder::Shaft(float x, float ceiling, float length, SDL_Color color, float cone)
    {
        auto entity = Make("Light Shaft", {x, ceiling}, 0.0f, Group("Lights"));
        entity.GetComponent<TransformComponent>().rotation = -90.0f;
        auto &light = entity.AddComponent<LightComponent>();
        light.type = LightType::Spot;
        light.color = color;
        light.radius = length;
        light.intensity = 1.25f;
        light.falloff = 0.75f;
        light.outerAngle = cone;
        light.innerAngle = cone * 0.35f;

        float spread = std::tan(glm::radians(cone)) * length;
        SIMPEngine::ParticleEffect dust;
        dust.Name = "Shaft Dust";
        dust.Emitter = SIMPEngine::EmitterShape::Box;
        dust.BoxSize = {spread * 0.9f, length * 0.75f};
        dust.Rate = 7.0f;
        dust.MaxParticles = 70;
        dust.Prewarm = true;
        dust.Lifetime = {5.0f, 8.0f};
        dust.Spread = 180.0f;
        dust.Speed = {3.0f, 10.0f};
        dust.Gravity = {0.0f, -2.0f};
        dust.Turbulence = 6.0f;
        dust.TurbulenceFrequency = 0.6f;
        dust.Size = {3.0f, 5.5f};
        dust.EndSize = 0.6f;
        dust.StartColor = ToVec4(color, 0.55f);
        dust.EndColor = ToVec4(color, 0.0f);
        dust.FadeIn = 0.3f;
        dust.Shape = SIMPEngine::ParticleShape::Soft;
        dust.Blend = SIMPEngine::BlendMode::Additive;
        dust.Emissive = true;
        Emitter("Shaft Dust", {x, ceiling - length * 0.45f}, dust, 4.0f, Group("Effects"));
    }

    void Builder::Fireflies(glm::vec2 center, glm::vec2 size, int count)
    {
        SIMPEngine::ParticleEffect effect = SIMPEngine::ParticlePresets::Find("Fireflies")->Effect;
        effect.BoxSize = size;
        effect.Rate = static_cast<float>(count) / 4.0f;
        effect.MaxParticles = count * 2;
        effect.Size = {7.0f, 11.0f};
        effect.StartColor = ToVec4(m_Palette.Accent, 1.0f);
        effect.EndColor = ToVec4(m_Palette.Accent, 0.0f);
        Emitter("Fireflies", center, effect, 4.5f, Group("Effects"));
    }

    void Builder::Spores(glm::vec2 center, glm::vec2 size)
    {
        SIMPEngine::ParticleEffect spores;
        spores.Name = "Spores";
        spores.Emitter = SIMPEngine::EmitterShape::Box;
        spores.BoxSize = size;
        spores.Rate = size.x * size.y / 90000.0f;
        spores.MaxParticles = 200;
        spores.Prewarm = true;
        spores.Lifetime = {6.0f, 10.0f};
        spores.Direction = 90.0f;
        spores.Spread = 60.0f;
        spores.Speed = {6.0f, 18.0f};
        spores.Turbulence = 12.0f;
        spores.TurbulenceFrequency = 0.5f;
        spores.Size = {2.5f, 5.0f};
        spores.EndSize = 0.5f;
        spores.StartColor = ToVec4(m_Palette.Cool, 0.7f);
        spores.EndColor = ToVec4(m_Palette.Accent, 0.0f);
        spores.FadeIn = 0.25f;
        spores.Shape = SIMPEngine::ParticleShape::Soft;
        spores.Blend = SIMPEngine::BlendMode::Additive;
        spores.Emissive = true;
        Emitter("Spores", center, spores, 4.6f, Group("Effects"));
    }

    void Builder::Mist(float x0, float x1, float y)
    {
        SIMPEngine::ParticleEffect mist;
        mist.Name = "Ground Mist";
        mist.Emitter = SIMPEngine::EmitterShape::Line;
        mist.BoxSize = {x1 - x0, 0.0f};
        mist.Rate = (x1 - x0) / 1100.0f;
        mist.MaxParticles = std::max(4, static_cast<int>((x1 - x0) / 140.0f));
        mist.Prewarm = true;
        mist.Lifetime = {9.0f, 14.0f};
        mist.Direction = 0.0f;
        mist.Spread = 15.0f;
        mist.Speed = {8.0f, 22.0f};
        mist.Size = {180.0f, 300.0f};
        mist.EndSize = 1.5f;
        mist.StartRotation = {0.0f, 360.0f};
        mist.AngularVelocity = {-6.0f, 6.0f};
        mist.StartColor = ToVec4(m_Palette.Fog, 0.22f);
        mist.EndColor = ToVec4(m_Palette.Fog, 0.0f);
        mist.FadeIn = 0.3f;
        mist.Shape = SIMPEngine::ParticleShape::Smoke;
        Emitter("Ground Mist", {(x0 + x1) * 0.5f, y + 30.0f}, mist, 5.5f, Group("Effects"));
    }

    void Builder::Drips(float x0, float x1, float ceiling)
    {
        SIMPEngine::ParticleEffect drips;
        drips.Name = "Drips";
        drips.Emitter = SIMPEngine::EmitterShape::Line;
        drips.BoxSize = {x1 - x0, 0.0f};
        drips.Rate = (x1 - x0) / 500.0f;
        drips.MaxParticles = 40;
        drips.Lifetime = {1.2f, 1.6f};
        drips.Direction = 270.0f;
        drips.Spread = 0.0f;
        drips.Speed = {0.0f, 20.0f};
        drips.Gravity = {0.0f, -900.0f};
        drips.Size = {3.0f, 4.0f};
        drips.EndSize = 1.0f;
        drips.Stretch = 0.02f;
        drips.AlignToVelocity = true;
        drips.StartColor = ToVec4(m_Palette.Cool, 0.7f);
        drips.EndColor = ToVec4(m_Palette.Cool, 0.2f);
        drips.Shape = SIMPEngine::ParticleShape::Spark;
        drips.Blend = SIMPEngine::BlendMode::Additive;
        drips.Emissive = true;
        Emitter("Drips", {(x0 + x1) * 0.5f, ceiling - 10.0f}, drips, 4.0f, Group("Effects"));
    }

    void Builder::Wisp(glm::vec2 at)
    {
        SIMPEngine::ParticleEffect core;
        core.Name = "Wisp";
        core.Emitter = SIMPEngine::EmitterShape::Circle;
        core.Radius = 6.0f;
        core.Rate = 45.0f;
        core.MaxParticles = 90;
        core.Prewarm = true;
        core.WorldSpace = false;
        core.Lifetime = {0.5f, 0.9f};
        core.Spread = 180.0f;
        core.Speed = {6.0f, 22.0f};
        core.Size = {12.0f, 22.0f};
        core.EndSize = 0.0f;
        core.StartColor = {0.9f, 1.0f, 1.0f, 0.95f};
        core.EndColor = ToVec4(m_Palette.Cool, 0.0f);
        core.Shape = SIMPEngine::ParticleShape::Soft;
        core.Blend = SIMPEngine::BlendMode::Additive;
        core.Emissive = true;
        auto wisp = Emitter("Wisp", at, core, 4.8f, Group("Pickups"));

        auto &light = wisp.AddComponent<LightComponent>();
        light.color = m_Palette.Cool;
        light.radius = 230.0f;
        light.intensity = 1.0f;
        light.falloff = 1.3f;
        light.flicker = 0.12f;
        light.flickerSpeed = 5.0f;

        auto &glow = wisp.AddComponent<GlowComponent>();
        glow.color = m_Palette.Cool;
        glow.intensity = 1.2f;
        glow.spread = 1.2f;
    }

    void Builder::Checkpoint(float x, float ground)
    {
        float height = 175.0f;
        auto flower = Make("Checkpoint", {x, ground + height * 0.46f}, 1.9f, Group("Pickups"));
        auto &sprite = flower.AddComponent<SpriteComponent>();
        sprite.width = height * Sheets::FlowerBlue.FrameWidth / static_cast<float>(Sheets::FlowerBlue.FrameHeight);
        sprite.height = height;
        sprite.tint = {150, 170, 190, 255};
        auto &anim = flower.AddComponent<AnimatedSpriteComponent>();
        anim.animation = std::make_unique<SIMPEngine::Animation>(Tex(SheetPath(Sheets::FlowerBlue)), Sheets::FlowerBlue.FrameWidth, Sheets::FlowerBlue.FrameHeight,
                                                                 Sheets::FlowerBlue.Frames, 0.05f, true);
        auto &light = flower.AddComponent<LightComponent>();
        light.color = {120, 190, 255, 255};
        light.radius = 340.0f;
        light.intensity = 0.25f;
        light.falloff = 1.1f;
        auto &glow = flower.AddComponent<GlowComponent>();
        glow.color = {120, 200, 255, 255};
        glow.intensity = 0.25f;
    }

    void Builder::Exit(float x, float ground)
    {
        SIMPEngine::ParticleEffect portal = SIMPEngine::ParticlePresets::Find("Portal")->Effect;
        portal.Radius = 110.0f;
        portal.Size = {5.0f, 9.0f};
        portal.StartColor = ToVec4(m_Palette.Cool, 1.0f);
        portal.MidColor = {0.75f, 0.55f, 1.0f, 0.8f};
        auto exit = Emitter("Exit", {x, ground + 160.0f}, portal, 4.7f, Group("Pickups"));
        auto &light = exit.AddComponent<LightComponent>();
        light.color = {170, 150, 255, 255};
        light.radius = 560.0f;
        light.intensity = 1.6f;
        light.falloff = 1.0f;
        light.flicker = 0.1f;
        light.flickerSpeed = 2.0f;
        auto &glow = exit.AddComponent<GlowComponent>();
        glow.color = {190, 170, 255, 255};
        glow.intensity = 1.4f;
        glow.spread = 1.4f;

        SIMPEngine::ParticleEffect rise = SIMPEngine::ParticlePresets::Find("Heal")->Effect;
        rise.Radius = 120.0f;
        rise.StartColor = {0.75f, 0.7f, 1.0f, 1.0f};
        rise.EndColor = {0.4f, 0.9f, 1.0f, 0.0f};
        Emitter("Exit Motes", {x, ground + 60.0f}, rise, 4.7f, Group("Effects"));
        Decor("decor_01", x - 210.0f, ground - 10.0f, 0.55f, 1.4f, {200, 200, 220, 255}, false);
        Decor("decor_01", x + 210.0f, ground - 10.0f, 0.5f, 1.4f, {200, 200, 220, 255}, true);
    }

    void Builder::DashShrine(float x, float ground)
    {
        Decor("decor_02", x, ground - 12.0f, 0.55f, 1.5f, {190, 200, 200, 255});
        SIMPEngine::ParticleEffect orbit = SIMPEngine::ParticlePresets::Find("Magic Orbit")->Effect;
        orbit.StartColor = {1.0f, 0.85f, 0.45f, 1.0f};
        orbit.EndColor = {0.5f, 1.0f, 0.8f, 0.0f};
        auto shrine = Emitter("Dash Shrine", {x, ground + 130.0f}, orbit, 4.7f, Group("Pickups"));
        auto &light = shrine.AddComponent<LightComponent>();
        light.color = {255, 210, 140, 255};
        light.radius = 420.0f;
        light.intensity = 1.3f;
        light.flicker = 0.15f;
        auto &glow = shrine.AddComponent<GlowComponent>();
        glow.color = {255, 220, 150, 255};
        glow.intensity = 1.3f;
    }

    void Builder::Player(glm::vec2 spawn)
    {
        auto player = Make("Player", spawn, 5.0f, entt::null);
        auto &sprite = player.AddComponent<SpriteComponent>();
        sprite.texture = Tex(SheetPath(Sheets::Idle));
        sprite.width = 108.0f;
        sprite.height = 115.0f;
        sprite.useRegion = true;
        sprite.region = SheetFrame(Sheets::Idle, 0);
        auto &glow = player.AddComponent<GlowComponent>();
        glow.color = {110, 160, 255, 255};
        glow.intensity = 0.3f;
        glow.spread = 1.0f;
        player.AddComponent<ScriptComponent>().Bind<PlayerScript>();

        auto light = Make("Player Light", {0.0f, 10.0f}, 0.0f, player.GetHandle());
        auto &l = light.AddComponent<LightComponent>();
        l.color = {190, 215, 255, 255};
        l.radius = 430.0f;
        l.intensity = 0.85f;
        l.falloff = 1.1f;

        SIMPEngine::ParticleEffect trail;
        trail.Name = "Dash Trail";
        trail.Emitter = SIMPEngine::EmitterShape::Box;
        trail.BoxSize = {30.0f, 80.0f};
        trail.Rate = 0.0f;
        trail.MaxParticles = 160;
        trail.Lifetime = {0.25f, 0.45f};
        trail.Spread = 180.0f;
        trail.Speed = {5.0f, 30.0f};
        trail.Size = {14.0f, 26.0f};
        trail.EndSize = 0.0f;
        trail.StartColor = {0.6f, 0.8f, 1.0f, 0.8f};
        trail.EndColor = {0.4f, 0.5f, 1.0f, 0.0f};
        trail.Shape = SIMPEngine::ParticleShape::Soft;
        trail.Blend = SIMPEngine::BlendMode::Additive;
        trail.Emissive = true;
        Emitter("Player Trail", {0.0f, -5.0f}, trail, 4.9f, player.GetHandle());
    }

    void Builder::Camera(glm::vec2 spawn, glm::vec2 boundsMin, glm::vec2 boundsMax)
    {
        auto camera = Make("Camera", spawn + glm::vec2(0.0f, 200.0f), 0.0f, entt::null);
        auto &cc = camera.AddComponent<CameraComponent>(0.75f, spawn + glm::vec2(0.0f, 200.0f));
        cc.primary = true;
        cc.followTarget = "Player";
        cc.followOffset = {0.0f, 200.0f};
        cc.followSmoothing = 4.5f;
        cc.deadZone = {110.0f, 80.0f};
        cc.useBounds = true;
        cc.boundsMin = boundsMin;
        cc.boundsMax = boundsMax;
        cc.shakeStrength = 14.0f;
        camera.AddComponent<ScriptComponent>().Bind<CameraScript>();

        State().BoundsMin = boundsMin;
        State().BoundsMax = boundsMax;
    }

    void Builder::Director(int level, uint32_t seed)
    {
        char name[64];
        std::snprintf(name, sizeof(name), "Director %d %u", level, seed);
        auto director = Make(name, {0.0f, 0.0f}, 0.0f, entt::null);
        director.AddComponent<ScriptComponent>().Bind<DirectorScript>();

        for (const char *lightName : {"HUD Light", "Card Light"})
        {
            auto entity = Make(lightName, {0.0f, 0.0f}, 0.0f, Group("HUD"));
            auto &light = entity.AddComponent<LightComponent>();
            light.color = {225, 235, 255, 255};
            light.radius = 340.0f;
            light.intensity = 0.0f;
            light.falloff = 0.35f;
        }
    }

    void BuildLevel(SIMPEngine::Scene &scene, int level, uint32_t seed)
    {
        scene.Clear();
        scene.SetName("Mossveil");
        Builder builder(scene, seed * 2654435761u + static_cast<uint32_t>(level));
        switch (level)
        {
        case 1:
            BuildMossway(builder);
            break;
        case 2:
            BuildThornroot(builder);
            break;
        default:
            BuildShiftingDeep(builder, seed);
            break;
        }
        builder.Director(level, seed);
        scene.UpdateTransforms();
    }
}
