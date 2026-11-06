#pragma once

#include <Engine/Rendering/RenderingAPI.h>
#include <Engine/Rendering/Texture.h>

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace SIMPEngine
{
    enum class EmitterShape : uint8_t
    {
        Point,
        Circle,
        Box,
        Line
    };

    enum class ParticleShape : uint8_t
    {
        Soft,
        Circle,
        Square,
        Spark,
        Star,
        Smoke,
        Ring,
        Texture
    };

    struct ParticleEffect
    {
        std::string Name = "Particles";

        bool Looping = true;
        float Duration = 2.0f;
        float Rate = 30.0f;
        int Burst = 0;
        int MaxParticles = 500;
        bool Prewarm = false;
        bool WorldSpace = true;

        EmitterShape Emitter = EmitterShape::Point;
        float Radius = 16.0f;
        glm::vec2 BoxSize{64.0f, 64.0f};
        bool EmitFromEdge = false;

        glm::vec2 Lifetime{0.8f, 1.2f};
        float Direction = 90.0f;
        float Spread = 25.0f;
        glm::vec2 Speed{80.0f, 140.0f};
        glm::vec2 Gravity{0.0f, 0.0f};
        float Drag = 0.0f;
        float Radial = 0.0f;
        float Orbital = 0.0f;
        float Turbulence = 0.0f;
        float TurbulenceFrequency = 1.5f;

        glm::vec2 Size{8.0f, 14.0f};
        float EndSize = 0.0f;
        float Stretch = 0.0f;

        glm::vec2 StartRotation{0.0f, 0.0f};
        glm::vec2 AngularVelocity{0.0f, 0.0f};
        bool AlignToVelocity = false;

        glm::vec4 StartColor{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 MidColor{1.0f, 1.0f, 1.0f, 0.5f};
        glm::vec4 EndColor{1.0f, 1.0f, 1.0f, 0.0f};
        bool UseMidColor = false;
        float HueVariation = 0.0f;
        float FadeIn = 0.0f;

        ParticleShape Shape = ParticleShape::Soft;
        std::shared_ptr<Texture> CustomTexture;
        BlendMode Blend = BlendMode::Alpha;
    };

    struct Particle
    {
        glm::vec2 Position{0.0f};
        glm::vec2 Velocity{0.0f};
        float Rotation = 0.0f;
        float AngularVelocity = 0.0f;
        float Age = 0.0f;
        float Life = 1.0f;
        float Size = 1.0f;
        float Hue = 0.0f;
        float Seed = 0.0f;
    };

    struct ParticleState
    {
        std::vector<Particle> Particles;
        float Time = 0.0f;
        float Accumulator = 0.0f;
        bool Started = false;
        bool Finished = false;
        uint32_t Rng = 0x9E3779B9u;
    };
}
