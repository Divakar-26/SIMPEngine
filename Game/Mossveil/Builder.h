#pragma once

#include "Common.h"

#include <Engine/Particles/ParticleEffect.h>

#include <random>

namespace Mossveil
{
    enum Edge : uint8_t
    {
        EdgeTop = 1,
        EdgeBottom = 2,
        EdgeLeft = 4,
        EdgeRight = 8,
        EdgeAll = 15
    };

    struct Palette
    {
        glm::vec3 Background{0.28f, 0.42f, 0.40f};
        glm::vec3 Ambient{0.30f, 0.38f, 0.40f};
        SDL_Color Warm{255, 196, 128, 255};
        SDL_Color Cool{120, 220, 255, 255};
        SDL_Color Accent{150, 255, 190, 255};
        SDL_Color Silhouette{70, 96, 100, 255};
        SDL_Color Fog{150, 210, 205, 26};
    };

    class Builder
    {
    public:
        Builder(SIMPEngine::Scene &scene, uint32_t seed);

        float Random();
        float Range(float a, float b);
        int Pick(int count);

        void Atmosphere(const Palette &palette, float bloomThreshold = 0.72f, float bloomIntensity = 0.55f);
        const Palette &Colors() const { return m_Palette; }

        void MossRect(glm::vec2 min, glm::vec2 max, uint8_t edges, const char *tag = "Solid");
        void Ground(float x0, float x1, float top, float bottom = -2600.0f, uint8_t sides = EdgeLeft | EdgeRight);
        void Ceiling(float x0, float x1, float bottom, float top = 3200.0f, uint8_t sides = EdgeLeft | EdgeRight);
        void Wall(float x0, float x1, float y0, float y1);
        void Platform(float cx, float top, float width);

        void Thorns(float cx, float surface, float width);
        void Plant(const SheetInfo &sheet, float x, float ground, float height, float z, SDL_Color tint = {255, 255, 255, 255});
        void GlowPlant(const SheetInfo &sheet, float x, float ground, float height, SDL_Color light, float z = 1.5f);
        void Decor(const std::string &piece, float x, float ground, float scale, float z, SDL_Color tint = {255, 255, 255, 255}, bool flip = false);
        void Hanging(const std::string &piece, float x, float ceiling, float scale, float z, SDL_Color tint = {255, 255, 255, 255});
        void Foliage(float x0, float x1, float ground, float density);
        void HangingFoliage(float x0, float x1, float ceiling, float density);

        void Background(float x0, float x1, float baseY, float factor, float z, SDL_Color tint, float spacing, float minScale, float maxScale);
        void Fog(float x0, float x1, float y, float height, float factor, float z, SDL_Color color);

        void Light(glm::vec2 at, SDL_Color color, float radius, float intensity, float falloff = 1.0f, float flicker = 0.0f);
        void Shaft(float x, float ceiling, float length, SDL_Color color, float cone = 16.0f);
        void Fireflies(glm::vec2 center, glm::vec2 size, int count);
        void Spores(glm::vec2 center, glm::vec2 size);
        void Mist(float x0, float x1, float y);
        void Drips(float x0, float x1, float ceiling);

        void Wisp(glm::vec2 at);
        void Checkpoint(float x, float ground);
        void Exit(float x, float ground);
        void DashShrine(float x, float ground);
        void Player(glm::vec2 spawn);
        void Camera(glm::vec2 spawn, glm::vec2 boundsMin, glm::vec2 boundsMax);
        void Director(int level, uint32_t seed);

    private:
        SIMPEngine::Entity Make(const char *name, glm::vec2 position, float z, entt::entity group);
        entt::entity Group(const char *name);
        SIMPEngine::Entity Emitter(const char *name, glm::vec2 at, const SIMPEngine::ParticleEffect &effect, float z, entt::entity group);

        SIMPEngine::Scene &m_Scene;
        std::mt19937 m_Rng;
        Palette m_Palette;
        std::vector<std::pair<std::string, entt::entity>> m_Groups;
    };

    void BuildLevel(SIMPEngine::Scene &scene, int level, uint32_t seed);
}
