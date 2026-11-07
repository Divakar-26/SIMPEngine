#include <Engine/Particles/ParticlePresets.h>

#include <algorithm>

namespace SIMPEngine
{
    namespace
    {
        ParticlePreset Make(const char *name, const char *category, const char *description, float viewHeight, glm::vec2 anchor)
        {
            ParticlePreset preset;
            preset.Name = name;
            preset.Category = category;
            preset.Description = description;
            preset.Effect.Name = name;
            preset.ViewHeight = viewHeight;
            preset.Anchor = anchor;
            return preset;
        }

        std::vector<ParticlePreset> Build()
        {
            std::vector<ParticlePreset> list;

            {
                auto p = Make("Fire", "Fire & Smoke", "Rising additive flames with flicker", 220.0f, {0.0f, -0.35f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Circle;
                e.Radius = 10.0f;
                e.Rate = 90.0f;
                e.MaxParticles = 300;
                e.Lifetime = {0.6f, 1.1f};
                e.Direction = 90.0f;
                e.Spread = 12.0f;
                e.Speed = {50.0f, 110.0f};
                e.Gravity = {0.0f, 40.0f};
                e.Drag = 0.5f;
                e.Turbulence = 60.0f;
                e.TurbulenceFrequency = 3.0f;
                e.Size = {22.0f, 34.0f};
                e.EndSize = 0.15f;
                e.StartColor = {1.0f, 0.85f, 0.35f, 0.9f};
                e.MidColor = {1.0f, 0.35f, 0.08f, 0.7f};
                e.EndColor = {0.5f, 0.05f, 0.02f, 0.0f};
                e.UseMidColor = true;
                e.Shape = ParticleShape::Soft;
                e.Blend = BlendMode::Additive;
                list.push_back(p);
            }
            {
                auto p = Make("Smoke", "Fire & Smoke", "Slow, billowing grey puffs", 300.0f, {0.0f, -0.4f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Circle;
                e.Radius = 12.0f;
                e.Rate = 18.0f;
                e.MaxParticles = 200;
                e.Prewarm = true;
                e.Lifetime = {2.5f, 3.5f};
                e.Direction = 90.0f;
                e.Spread = 15.0f;
                e.Speed = {25.0f, 45.0f};
                e.Gravity = {0.0f, 8.0f};
                e.Drag = 0.3f;
                e.Turbulence = 25.0f;
                e.TurbulenceFrequency = 1.0f;
                e.Size = {22.0f, 32.0f};
                e.EndSize = 3.0f;
                e.StartRotation = {0.0f, 360.0f};
                e.AngularVelocity = {-30.0f, 30.0f};
                e.StartColor = {0.55f, 0.55f, 0.58f, 0.55f};
                e.EndColor = {0.35f, 0.35f, 0.38f, 0.0f};
                e.FadeIn = 0.15f;
                e.Shape = ParticleShape::Smoke;
                list.push_back(p);
            }
            {
                auto p = Make("Embers", "Fire & Smoke", "Drifting glowing specks", 260.0f, {0.0f, -0.4f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Box;
                e.BoxSize = {100.0f, 10.0f};
                e.Rate = 25.0f;
                e.Prewarm = true;
                e.Lifetime = {1.5f, 3.0f};
                e.Direction = 90.0f;
                e.Spread = 25.0f;
                e.Speed = {30.0f, 70.0f};
                e.Gravity = {0.0f, 10.0f};
                e.Turbulence = 80.0f;
                e.TurbulenceFrequency = 2.0f;
                e.Size = {3.0f, 6.0f};
                e.EndSize = 0.3f;
                e.StartColor = {1.0f, 0.7f, 0.25f, 1.0f};
                e.EndColor = {1.0f, 0.25f, 0.05f, 0.0f};
                e.FadeIn = 0.1f;
                e.Shape = ParticleShape::Soft;
                e.Blend = BlendMode::Additive;
                list.push_back(p);
            }
            {
                auto p = Make("Sparks", "Impact", "Bursts of hot streaks that fall", 260.0f, {0.0f, -0.1f});
                auto &e = p.Effect;
                e.Duration = 0.9f;
                e.Rate = 0.0f;
                e.Burst = 40;
                e.Lifetime = {0.35f, 0.8f};
                e.Direction = 90.0f;
                e.Spread = 70.0f;
                e.Speed = {180.0f, 380.0f};
                e.Gravity = {0.0f, -600.0f};
                e.Drag = 1.0f;
                e.Size = {4.0f, 6.5f};
                e.EndSize = 0.5f;
                e.Stretch = 0.045f;
                e.AlignToVelocity = true;
                e.StartColor = {1.0f, 0.95f, 0.6f, 1.0f};
                e.EndColor = {1.0f, 0.45f, 0.1f, 0.0f};
                e.Shape = ParticleShape::Spark;
                e.Blend = BlendMode::Additive;
                list.push_back(p);
            }
            {
                auto p = Make("Explosion", "Impact", "Fireball that expands and fades to smoke", 280.0f, {0.0f, 0.0f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Circle;
                e.Radius = 6.0f;
                e.Duration = 1.6f;
                e.Rate = 0.0f;
                e.Burst = 70;
                e.Lifetime = {0.5f, 1.1f};
                e.Spread = 180.0f;
                e.Speed = {60.0f, 260.0f};
                e.Drag = 3.5f;
                e.Size = {18.0f, 34.0f};
                e.EndSize = 1.8f;
                e.StartColor = {1.0f, 0.9f, 0.5f, 1.0f};
                e.MidColor = {1.0f, 0.4f, 0.1f, 0.8f};
                e.EndColor = {0.25f, 0.2f, 0.2f, 0.0f};
                e.UseMidColor = true;
                e.Shape = ParticleShape::Soft;
                e.Blend = BlendMode::Additive;
                list.push_back(p);
            }
            {
                auto p = Make("Dust Puff", "Impact", "Landing or footstep dust", 160.0f, {0.0f, -0.3f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Line;
                e.BoxSize = {40.0f, 0.0f};
                e.Duration = 1.4f;
                e.Rate = 0.0f;
                e.Burst = 18;
                e.Lifetime = {0.6f, 1.0f};
                e.Direction = 90.0f;
                e.Spread = 80.0f;
                e.Speed = {30.0f, 80.0f};
                e.Gravity = {0.0f, -30.0f};
                e.Drag = 2.0f;
                e.Size = {10.0f, 18.0f};
                e.EndSize = 2.0f;
                e.StartRotation = {0.0f, 360.0f};
                e.StartColor = {0.75f, 0.68f, 0.55f, 0.6f};
                e.EndColor = {0.75f, 0.68f, 0.55f, 0.0f};
                e.Shape = ParticleShape::Smoke;
                list.push_back(p);
            }
            {
                auto p = Make("Shockwave", "Impact", "Expanding rings for hits", 220.0f, {0.0f, 0.0f});
                auto &e = p.Effect;
                e.Duration = 1.0f;
                e.Rate = 0.0f;
                e.Burst = 3;
                e.Lifetime = {0.4f, 0.6f};
                e.Speed = {0.0f, 0.0f};
                e.Size = {16.0f, 20.0f};
                e.EndSize = 9.0f;
                e.StartColor = {1.0f, 1.0f, 1.0f, 0.9f};
                e.EndColor = {0.6f, 0.8f, 1.0f, 0.0f};
                e.Shape = ParticleShape::Ring;
                e.Blend = BlendMode::Additive;
                list.push_back(p);
            }
            {
                auto p = Make("Magic Orbit", "Magic", "Stars circling an arcane core", 200.0f, {0.0f, 0.0f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Circle;
                e.Radius = 40.0f;
                e.EmitFromEdge = true;
                e.Rate = 40.0f;
                e.Prewarm = true;
                e.Lifetime = {1.0f, 1.6f};
                e.Spread = 180.0f;
                e.Speed = {5.0f, 20.0f};
                e.Radial = -20.0f;
                e.Orbital = 90.0f;
                e.Size = {8.0f, 14.0f};
                e.EndSize = 0.0f;
                e.AngularVelocity = {-90.0f, 90.0f};
                e.StartColor = {0.6f, 0.5f, 1.0f, 1.0f};
                e.EndColor = {0.3f, 0.9f, 1.0f, 0.0f};
                e.HueVariation = 0.15f;
                e.Shape = ParticleShape::Star;
                e.Blend = BlendMode::Additive;
                list.push_back(p);
            }
            {
                auto p = Make("Heal", "Magic", "Soft green sparkles rising", 200.0f, {0.0f, -0.3f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Circle;
                e.Radius = 30.0f;
                e.Rate = 25.0f;
                e.Prewarm = true;
                e.Lifetime = {1.0f, 1.8f};
                e.Direction = 90.0f;
                e.Spread = 10.0f;
                e.Speed = {25.0f, 50.0f};
                e.Size = {8.0f, 14.0f};
                e.EndSize = 0.2f;
                e.StartColor = {0.5f, 1.0f, 0.6f, 1.0f};
                e.EndColor = {0.2f, 1.0f, 0.5f, 0.0f};
                e.FadeIn = 0.2f;
                e.Shape = ParticleShape::Star;
                e.Blend = BlendMode::Additive;
                list.push_back(p);
            }
            {
                auto p = Make("Portal", "Magic", "Particles spiralling into a vortex", 240.0f, {0.0f, 0.0f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Circle;
                e.Radius = 90.0f;
                e.EmitFromEdge = true;
                e.Rate = 120.0f;
                e.Prewarm = true;
                e.Lifetime = {0.9f, 1.2f};
                e.Spread = 180.0f;
                e.Speed = {0.0f, 5.0f};
                e.Drag = 2.8f;
                e.Radial = -420.0f;
                e.Orbital = 260.0f;
                e.Size = {4.0f, 7.0f};
                e.EndSize = 0.3f;
                e.StartColor = {0.4f, 0.3f, 1.0f, 1.0f};
                e.MidColor = {0.8f, 0.4f, 1.0f, 0.8f};
                e.EndColor = {1.0f, 1.0f, 1.0f, 0.0f};
                e.UseMidColor = true;
                e.Shape = ParticleShape::Soft;
                e.Blend = BlendMode::Additive;
                list.push_back(p);
            }
            {
                auto p = Make("Snow", "Weather", "Gently drifting snowflakes", 280.0f, {0.0f, 0.48f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Line;
                e.BoxSize = {360.0f, 0.0f};
                e.Rate = 30.0f;
                e.Prewarm = true;
                e.Lifetime = {5.0f, 7.0f};
                e.Direction = 270.0f;
                e.Spread = 10.0f;
                e.Speed = {30.0f, 50.0f};
                e.Turbulence = 20.0f;
                e.TurbulenceFrequency = 0.8f;
                e.Size = {3.0f, 7.0f};
                e.EndSize = 1.0f;
                e.StartColor = {1.0f, 1.0f, 1.0f, 0.9f};
                e.EndColor = {1.0f, 1.0f, 1.0f, 0.0f};
                e.FadeIn = 0.1f;
                e.Shape = ParticleShape::Circle;
                list.push_back(p);
            }
            {
                auto p = Make("Rain", "Weather", "Fast slanted rain streaks", 280.0f, {0.0f, 0.5f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Line;
                e.BoxSize = {380.0f, 0.0f};
                e.Rate = 140.0f;
                e.Prewarm = true;
                e.Lifetime = {0.6f, 0.8f};
                e.Direction = 255.0f;
                e.Spread = 2.0f;
                e.Speed = {520.0f, 620.0f};
                e.Size = {2.5f, 3.5f};
                e.EndSize = 1.0f;
                e.Stretch = 0.03f;
                e.AlignToVelocity = true;
                e.StartColor = {0.65f, 0.75f, 0.95f, 0.65f};
                e.EndColor = {0.65f, 0.75f, 0.95f, 0.5f};
                e.Shape = ParticleShape::Spark;
                list.push_back(p);
            }
            {
                auto p = Make("Fireflies", "Nature", "Wandering glowing bugs", 220.0f, {0.0f, 0.0f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Box;
                e.BoxSize = {260.0f, 160.0f};
                e.Rate = 6.0f;
                e.Prewarm = true;
                e.Lifetime = {3.0f, 5.0f};
                e.Spread = 180.0f;
                e.Speed = {5.0f, 15.0f};
                e.Turbulence = 40.0f;
                e.TurbulenceFrequency = 0.7f;
                e.Size = {8.0f, 13.0f};
                e.EndSize = 0.6f;
                e.StartColor = {0.85f, 1.0f, 0.35f, 1.0f};
                e.EndColor = {0.6f, 1.0f, 0.2f, 0.0f};
                e.FadeIn = 0.3f;
                e.Shape = ParticleShape::Soft;
                e.Blend = BlendMode::Additive;
                list.push_back(p);
            }
            {
                auto p = Make("Fountain", "Nature", "Water jet arcing under gravity", 300.0f, {0.0f, -0.45f});
                auto &e = p.Effect;
                e.Rate = 120.0f;
                e.Prewarm = true;
                e.Lifetime = {1.2f, 1.6f};
                e.Direction = 90.0f;
                e.Spread = 13.0f;
                e.Speed = {270.0f, 330.0f};
                e.Gravity = {0.0f, -420.0f};
                e.Size = {4.0f, 7.0f};
                e.EndSize = 0.6f;
                e.StartColor = {0.55f, 0.8f, 1.0f, 0.9f};
                e.EndColor = {0.3f, 0.55f, 1.0f, 0.0f};
                e.Shape = ParticleShape::Circle;
                list.push_back(p);
            }
            {
                auto p = Make("Bubbles", "Nature", "Wobbling bubbles floating up", 260.0f, {0.0f, -0.45f});
                auto &e = p.Effect;
                e.Emitter = EmitterShape::Line;
                e.BoxSize = {120.0f, 0.0f};
                e.Rate = 8.0f;
                e.Prewarm = true;
                e.Lifetime = {2.5f, 4.0f};
                e.Direction = 90.0f;
                e.Spread = 8.0f;
                e.Speed = {30.0f, 60.0f};
                e.Turbulence = 30.0f;
                e.TurbulenceFrequency = 1.2f;
                e.Size = {8.0f, 18.0f};
                e.EndSize = 1.2f;
                e.StartColor = {0.7f, 0.9f, 1.0f, 0.8f};
                e.EndColor = {0.7f, 0.9f, 1.0f, 0.0f};
                e.FadeIn = 0.1f;
                e.Shape = ParticleShape::Ring;
                list.push_back(p);
            }
            {
                auto p = Make("Confetti", "Celebration", "Colourful tumbling paper", 320.0f, {0.0f, -0.4f});
                auto &e = p.Effect;
                e.Duration = 2.2f;
                e.Rate = 0.0f;
                e.Burst = 90;
                e.Lifetime = {2.0f, 3.0f};
                e.Direction = 90.0f;
                e.Spread = 35.0f;
                e.Speed = {220.0f, 360.0f};
                e.Gravity = {0.0f, -260.0f};
                e.Drag = 1.4f;
                e.Size = {6.0f, 9.0f};
                e.EndSize = 1.0f;
                e.StartRotation = {0.0f, 360.0f};
                e.AngularVelocity = {-400.0f, 400.0f};
                e.StartColor = {1.0f, 0.3f, 0.3f, 1.0f};
                e.MidColor = {1.0f, 0.3f, 0.3f, 1.0f};
                e.EndColor = {1.0f, 0.3f, 0.3f, 0.0f};
                e.UseMidColor = true;
                e.HueVariation = 1.0f;
                e.Shape = ParticleShape::Square;
                list.push_back(p);
            }

            for (auto &preset : list)
                preset.Effect.Emissive = preset.Effect.Blend == BlendMode::Additive;
            return list;
        }
    }

    const std::vector<ParticlePreset> &ParticlePresets::All()
    {
        static const std::vector<ParticlePreset> s_Presets = Build();
        return s_Presets;
    }

    const std::vector<std::string> &ParticlePresets::Categories()
    {
        static const std::vector<std::string> s_Categories = []
        {
            std::vector<std::string> out;
            for (const auto &preset : All())
                if (std::find(out.begin(), out.end(), preset.Category) == out.end())
                    out.push_back(preset.Category);
            return out;
        }();
        return s_Categories;
    }

    const ParticlePreset *ParticlePresets::Find(const std::string &name)
    {
        for (const auto &preset : All())
            if (preset.Name == name)
                return &preset;
        return nullptr;
    }
}
