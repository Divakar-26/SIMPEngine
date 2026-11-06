#include <Engine/Particles/ParticleEmitter.h>
#include <Engine/Rendering/Renderer.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace SIMPEngine
{
    namespace
    {
        constexpr float Pi = 3.14159265358979f;

        float Random01(ParticleState &state)
        {
            uint32_t x = state.Rng;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            state.Rng = x ? x : 0x9E3779B9u;
            return static_cast<float>(x & 0xFFFFFFu) / static_cast<float>(0x1000000u);
        }

        float RandomRange(ParticleState &state, const glm::vec2 &range)
        {
            return range.x + (range.y - range.x) * Random01(state);
        }

        glm::vec2 Rotate(const glm::vec2 &v, float degrees)
        {
            float r = glm::radians(degrees);
            float c = std::cos(r), s = std::sin(r);
            return {v.x * c - v.y * s, v.x * s + v.y * c};
        }

        glm::vec2 SpawnOffset(const ParticleEffect &effect, ParticleState &state)
        {
            switch (effect.Emitter)
            {
            case EmitterShape::Circle:
            {
                float angle = Random01(state) * 2.0f * Pi;
                float r = effect.EmitFromEdge ? effect.Radius : effect.Radius * std::sqrt(Random01(state));
                return {std::cos(angle) * r, std::sin(angle) * r};
            }
            case EmitterShape::Box:
            {
                glm::vec2 half = effect.BoxSize * 0.5f;
                if (!effect.EmitFromEdge)
                    return {(Random01(state) * 2.0f - 1.0f) * half.x, (Random01(state) * 2.0f - 1.0f) * half.y};

                float perimeter = 2.0f * (effect.BoxSize.x + effect.BoxSize.y);
                float d = Random01(state) * std::max(perimeter, 0.0001f);
                if (d < effect.BoxSize.x)
                    return {d - half.x, -half.y};
                d -= effect.BoxSize.x;
                if (d < effect.BoxSize.y)
                    return {half.x, d - half.y};
                d -= effect.BoxSize.y;
                if (d < effect.BoxSize.x)
                    return {half.x - d, half.y};
                d -= effect.BoxSize.x;
                return {-half.x, half.y - d};
            }
            case EmitterShape::Line:
                return {(Random01(state) - 0.5f) * effect.BoxSize.x, 0.0f};
            case EmitterShape::Point:
            default:
                return {0.0f, 0.0f};
            }
        }

        void Spawn(const ParticleEffect &effect, ParticleState &state, const EmitterFrame &frame)
        {
            if (static_cast<int>(state.Particles.size()) >= std::max(1, effect.MaxParticles))
                return;

            Particle p;
            glm::vec2 offset = Rotate(SpawnOffset(effect, state), frame.Rotation) * frame.Scale;
            p.Position = effect.WorldSpace ? frame.Origin + offset : offset;

            float angle = effect.Direction + frame.Rotation + (Random01(state) * 2.0f - 1.0f) * effect.Spread;
            float speed = RandomRange(state, effect.Speed) * frame.Scale;
            float rad = glm::radians(angle);
            p.Velocity = {std::cos(rad) * speed, std::sin(rad) * speed};

            p.Life = std::max(0.01f, RandomRange(state, effect.Lifetime));
            p.Size = std::max(0.0f, RandomRange(state, effect.Size)) * frame.Scale;
            p.Rotation = RandomRange(state, effect.StartRotation);
            p.AngularVelocity = RandomRange(state, effect.AngularVelocity);
            p.Hue = (Random01(state) * 2.0f - 1.0f) * effect.HueVariation * 0.5f;
            p.Seed = Random01(state) * 628.0f;
            state.Particles.push_back(p);
        }

        glm::vec3 ShiftHue(const glm::vec3 &rgb, float shift)
        {
            float maxC = std::max({rgb.r, rgb.g, rgb.b});
            float minC = std::min({rgb.r, rgb.g, rgb.b});
            float delta = maxC - minC;
            if (delta <= 0.0001f)
                return rgb;

            float h;
            if (maxC == rgb.r)
                h = std::fmod((rgb.g - rgb.b) / delta, 6.0f);
            else if (maxC == rgb.g)
                h = (rgb.b - rgb.r) / delta + 2.0f;
            else
                h = (rgb.r - rgb.g) / delta + 4.0f;
            h = h / 6.0f + shift;
            h -= std::floor(h);

            float s = delta / maxC;
            float v = maxC;
            float c = v * s;
            float hp = h * 6.0f;
            float x = c * (1.0f - std::abs(std::fmod(hp, 2.0f) - 1.0f));
            glm::vec3 out;
            if (hp < 1.0f)
                out = {c, x, 0};
            else if (hp < 2.0f)
                out = {x, c, 0};
            else if (hp < 3.0f)
                out = {0, c, x};
            else if (hp < 4.0f)
                out = {0, x, c};
            else if (hp < 5.0f)
                out = {x, 0, c};
            else
                out = {c, 0, x};
            return out + glm::vec3(v - c);
        }

        glm::vec4 Gradient(const ParticleEffect &effect, float t)
        {
            if (!effect.UseMidColor)
                return glm::mix(effect.StartColor, effect.EndColor, t);
            if (t < 0.5f)
                return glm::mix(effect.StartColor, effect.MidColor, t * 2.0f);
            return glm::mix(effect.MidColor, effect.EndColor, (t - 0.5f) * 2.0f);
        }
    }

    void ParticleEmitter::Restart(ParticleState &state)
    {
        state.Particles.clear();
        state.Time = 0.0f;
        state.Accumulator = 0.0f;
        state.Started = false;
        state.Finished = false;
    }

    bool ParticleEmitter::IsAlive(const ParticleEffect &, const ParticleState &state)
    {
        return !state.Finished || !state.Particles.empty();
    }

    void ParticleEmitter::Emit(const ParticleEffect &effect, ParticleState &state, const EmitterFrame &frame, int count)
    {
        for (int i = 0; i < count; ++i)
            Spawn(effect, state, frame);
    }

    void ParticleEmitter::Prewarm(const ParticleEffect &effect, ParticleState &state, const EmitterFrame &frame, float seconds)
    {
        const float step = 1.0f / 30.0f;
        for (float t = 0.0f; t < seconds; t += step)
            Simulate(effect, state, frame, step, true);
    }

    void ParticleEmitter::Simulate(const ParticleEffect &effect, ParticleState &state, const EmitterFrame &frame, float dt, bool emitting)
    {
        if (!state.Started)
        {
            state.Started = true;
            state.Time = 0.0f;
            state.Accumulator = 0.0f;
            if (emitting && effect.Burst > 0)
                Emit(effect, state, frame, effect.Burst);
            if (effect.Prewarm && effect.Looping)
                Prewarm(effect, state, frame, std::min(effect.Lifetime.y, 10.0f));
        }

        dt = std::clamp(dt, 0.0f, 0.1f);

        glm::vec2 center = effect.WorldSpace ? frame.Origin : glm::vec2(0.0f);
        glm::vec2 gravity = effect.Gravity * frame.Scale;
        float drag = 1.0f / (1.0f + std::max(0.0f, effect.Drag) * dt);
        float freq = effect.TurbulenceFrequency;
        float clock = state.Time;

        auto &particles = state.Particles;
        for (size_t i = 0; i < particles.size();)
        {
            Particle &p = particles[i];
            p.Age += dt;
            if (p.Age >= p.Life)
            {
                p = particles.back();
                particles.pop_back();
                continue;
            }

            glm::vec2 accel = gravity;
            if (effect.Radial != 0.0f || effect.Orbital != 0.0f)
            {
                glm::vec2 d = p.Position - center;
                float len = glm::length(d);
                if (len > 0.001f)
                {
                    glm::vec2 n = d / len;
                    accel += n * effect.Radial * frame.Scale;
                    accel += glm::vec2(-n.y, n.x) * effect.Orbital * frame.Scale;
                }
            }
            if (effect.Turbulence != 0.0f)
            {
                float phase = clock * freq + p.Age * freq;
                glm::vec2 noise(std::sin(p.Seed + phase * 1.7f + p.Position.y * 0.013f),
                                std::cos(p.Seed * 1.3f + phase * 1.3f + p.Position.x * 0.011f));
                accel += noise * effect.Turbulence * frame.Scale;
            }

            p.Velocity = (p.Velocity + accel * dt) * drag;
            p.Position += p.Velocity * dt;
            p.Rotation += p.AngularVelocity * dt;
            ++i;
        }

        if (!emitting || state.Finished)
            return;

        float duration = std::max(0.01f, effect.Duration);
        state.Time += dt;

        if (state.Time >= duration)
        {
            if (effect.Looping)
            {
                state.Time = std::fmod(state.Time, duration);
                if (effect.Burst > 0)
                    Emit(effect, state, frame, effect.Burst);
            }
            else
            {
                state.Finished = true;
                return;
            }
        }

        state.Accumulator += std::max(0.0f, effect.Rate) * dt;
        int budget = std::max(1, effect.MaxParticles);
        while (state.Accumulator >= 1.0f && budget-- > 0)
        {
            Spawn(effect, state, frame);
            state.Accumulator -= 1.0f;
        }
        state.Accumulator = std::min(state.Accumulator, 1.0f);
    }

    void ParticleEmitter::Render(const ParticleEffect &effect, const ParticleState &state, const EmitterFrame &frame, float z)
    {
        if (state.Particles.empty())
            return;

        std::shared_ptr<Texture> texture;
        if (effect.Shape == ParticleShape::Texture && effect.CustomTexture && effect.CustomTexture->GetID())
            texture = effect.CustomTexture;
        else
            texture = ParticleTextures::Get(effect.Shape == ParticleShape::Texture ? ParticleShape::Soft : effect.Shape);
        if (!texture)
            return;

        BlendMode previous = Renderer::GetBlendMode();
        Renderer::SetBlendMode(effect.Blend);

        SpriteDesc desc;
        desc.TextureRef = texture;
        desc.Z = z;

        bool oriented = effect.AlignToVelocity || effect.Stretch > 0.0f;
        bool hue = effect.HueVariation > 0.0f;

        for (const Particle &p : state.Particles)
        {
            float t = std::clamp(p.Age / p.Life, 0.0f, 1.0f);
            glm::vec4 color = Gradient(effect, t);
            if (hue && p.Hue != 0.0f)
                color = glm::vec4(ShiftHue(glm::vec3(color), p.Hue), color.a);
            if (effect.FadeIn > 0.0f)
                color.a *= std::clamp(t / effect.FadeIn, 0.0f, 1.0f);
            if (color.a <= 0.004f)
                continue;

            float size = p.Size * glm::mix(1.0f, effect.EndSize, t);
            if (size <= 0.01f)
                continue;

            glm::vec2 extent(size, size);
            float rotation = p.Rotation;
            if (oriented)
            {
                float speed = glm::length(p.Velocity);
                if (speed > 0.001f)
                    rotation = glm::degrees(std::atan2(p.Velocity.y, p.Velocity.x));
                if (effect.Stretch > 0.0f)
                    extent.x = size + speed * effect.Stretch;
            }

            color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
            desc.Tint = {static_cast<Uint8>(color.r * 255.0f), static_cast<Uint8>(color.g * 255.0f),
                         static_cast<Uint8>(color.b * 255.0f), static_cast<Uint8>(color.a * 255.0f)};
            desc.Position = effect.WorldSpace ? p.Position : frame.Origin + p.Position;
            desc.Size = extent;
            desc.Rotation = rotation;
            Renderer::DrawSprite(desc);
        }

        Renderer::SetBlendMode(previous);
    }

    namespace
    {
        constexpr int TextureSize = 64;
        std::array<std::shared_ptr<Texture>, 7> s_Textures;

        float Hash(int x, int y)
        {
            uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
            h = (h ^ (h >> 13)) * 1274126177u;
            return static_cast<float>((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
        }

        float ValueNoise(float x, float y)
        {
            int xi = static_cast<int>(std::floor(x));
            int yi = static_cast<int>(std::floor(y));
            float fx = x - static_cast<float>(xi);
            float fy = y - static_cast<float>(yi);
            fx = fx * fx * (3.0f - 2.0f * fx);
            fy = fy * fy * (3.0f - 2.0f * fy);
            float a = Hash(xi, yi), b = Hash(xi + 1, yi), c = Hash(xi, yi + 1), d = Hash(xi + 1, yi + 1);
            return glm::mix(glm::mix(a, b, fx), glm::mix(c, d, fx), fy);
        }

        float ShapeAlpha(ParticleShape shape, float nx, float ny)
        {
            float r = std::sqrt(nx * nx + ny * ny);
            float px = 2.0f / static_cast<float>(TextureSize);
            switch (shape)
            {
            case ParticleShape::Soft:
            {
                float a = std::exp(-r * r * 4.5f);
                return a * (1.0f - glm::smoothstep(0.8f, 1.0f, r));
            }
            case ParticleShape::Circle:
                return std::clamp((0.94f - r) / px, 0.0f, 1.0f);
            case ParticleShape::Square:
            {
                float d = std::max(std::abs(nx), std::abs(ny));
                return std::clamp((0.9f - d) / px, 0.0f, 1.0f);
            }
            case ParticleShape::Spark:
            {
                float ex = nx;
                float ey = ny / 0.5f;
                float e = std::sqrt(ex * ex + ey * ey);
                float a = std::pow(std::max(0.0f, 1.0f - e), 1.4f);
                return std::clamp(a * 1.4f, 0.0f, 1.0f);
            }
            case ParticleShape::Star:
            {
                float core = std::exp(-r * r * 30.0f);
                float rayH = std::exp(-ny * ny * 260.0f) * std::max(0.0f, 1.0f - std::abs(nx));
                float rayV = std::exp(-nx * nx * 260.0f) * std::max(0.0f, 1.0f - std::abs(ny));
                float glow = std::exp(-r * r * 6.0f) * 0.35f;
                return std::clamp(core + rayH + rayV + glow, 0.0f, 1.0f) * (1.0f - glm::smoothstep(0.92f, 1.0f, r));
            }
            case ParticleShape::Smoke:
            {
                float n = ValueNoise(nx * 3.0f + 7.0f, ny * 3.0f + 3.0f) * 0.6f + ValueNoise(nx * 7.0f + 1.0f, ny * 7.0f + 9.0f) * 0.4f;
                float base = std::pow(std::max(0.0f, 1.0f - r), 1.3f);
                return std::clamp(base * (0.45f + 0.75f * n), 0.0f, 1.0f);
            }
            case ParticleShape::Ring:
            {
                float d = std::abs(r - 0.78f) - 0.1f;
                return std::clamp(-d / px, 0.0f, 1.0f);
            }
            default:
                return 0.0f;
            }
        }
    }

    std::shared_ptr<Texture> ParticleTextures::Get(ParticleShape shape)
    {
        size_t index = static_cast<size_t>(shape);
        if (index >= s_Textures.size())
            return nullptr;
        if (s_Textures[index])
            return s_Textures[index];

        std::vector<unsigned char> pixels(TextureSize * TextureSize * 4);
        for (int y = 0; y < TextureSize; ++y)
        {
            for (int x = 0; x < TextureSize; ++x)
            {
                float nx = (static_cast<float>(x) + 0.5f) / TextureSize * 2.0f - 1.0f;
                float ny = (static_cast<float>(y) + 0.5f) / TextureSize * 2.0f - 1.0f;
                float a = ShapeAlpha(shape, nx, ny);
                unsigned char *px = &pixels[static_cast<size_t>((y * TextureSize + x) * 4)];
                px[0] = px[1] = px[2] = 255;
                px[3] = static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
            }
        }

        auto texture = std::make_shared<Texture>();
        texture->UploadToGPU(pixels.data(), TextureSize, TextureSize, 4);
        s_Textures[index] = texture;
        return texture;
    }

    void ParticleTextures::Release()
    {
        for (auto &texture : s_Textures)
            texture.reset();
    }
}
