#include "Tween.h"

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Vector/VectorPath.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <limits>

namespace SIMPEngine
{
    namespace
    {
        constexpr float Infinite = std::numeric_limits<float>::infinity();

        uint64_t s_NextSerial = 1;
        std::unordered_map<uint64_t, TweenSystem *> &Systems()
        {
            static std::unordered_map<uint64_t, TweenSystem *> systems;
            return systems;
        }

        struct PropertyInfo
        {
            const char *Name;
            TweenProperty Property;
            int Channels;
        };

        const PropertyInfo Properties[] = {
            {"x", TweenProperty::X, 1},
            {"y", TweenProperty::Y, 1},
            {"position", TweenProperty::Position, 2},
            {"rotation", TweenProperty::Rotation, 1},
            {"scaleX", TweenProperty::ScaleX, 1},
            {"scaleY", TweenProperty::ScaleY, 1},
            {"scale", TweenProperty::Scale, 2},
            {"z", TweenProperty::Z, 1},
            {"color", TweenProperty::Color, 4},
            {"alpha", TweenProperty::Alpha, 1},
            {"width", TweenProperty::Width, 1},
            {"height", TweenProperty::Height, 1},
            {"size", TweenProperty::Size, 2},
            {"tint", TweenProperty::Tint, 4},
            {"shapeColor", TweenProperty::ShapeColor, 4},
            {"textColor", TweenProperty::TextColor, 4},
            {"fontSize", TweenProperty::FontSize, 1},
            {"lightColor", TweenProperty::LightColor, 4},
            {"lightIntensity", TweenProperty::LightIntensity, 1},
            {"lightRadius", TweenProperty::LightRadius, 1},
            {"glowColor", TweenProperty::GlowColor, 4},
            {"glowIntensity", TweenProperty::GlowIntensity, 1},
            {"glowSpread", TweenProperty::GlowSpread, 1},
            {"zoom", TweenProperty::Zoom, 1},
            {"particleRate", TweenProperty::ParticleRate, 1},
            {"volume", TweenProperty::Volume, 1},
            {"pitch", TweenProperty::Pitch, 1},
            {"animationSpeed", TweenProperty::AnimationSpeed, 1},
            {"fillColor", TweenProperty::FillColor, 4},
            {"strokeColor", TweenProperty::StrokeColor, 4},
            {"strokeWidth", TweenProperty::StrokeWidth, 1},
            {"morph", TweenProperty::Morph, 1},
            {"trimStart", TweenProperty::TrimStart, 1},
            {"trimEnd", TweenProperty::TrimEnd, 1},
            {"trimOffset", TweenProperty::TrimOffset, 1},
            {"dashOffset", TweenProperty::DashOffset, 1},
        };

        bool Overlaps(TweenProperty a, TweenProperty b)
        {
            if (a == b)
                return true;
            auto pair = [&](TweenProperty x, TweenProperty y)
            { return (a == x && b == y) || (a == y && b == x); };
            return pair(TweenProperty::X, TweenProperty::Position) || pair(TweenProperty::Y, TweenProperty::Position) ||
                   pair(TweenProperty::ScaleX, TweenProperty::Scale) || pair(TweenProperty::ScaleY, TweenProperty::Scale) ||
                   pair(TweenProperty::Width, TweenProperty::Size) || pair(TweenProperty::Height, TweenProperty::Size) ||
                   pair(TweenProperty::Alpha, TweenProperty::Color);
        }

        void ReadColor(const SDL_Color &c, float out[4])
        {
            out[0] = c.r;
            out[1] = c.g;
            out[2] = c.b;
            out[3] = c.a;
        }

        Uint8 Byte(float v)
        {
            return static_cast<Uint8>(std::clamp(std::round(v), 0.0f, 255.0f));
        }

        void WriteColor(SDL_Color &c, const float v[4])
        {
            c = SDL_Color{Byte(v[0]), Byte(v[1]), Byte(v[2]), Byte(v[3])};
        }

        struct MapResult
        {
            int Cycle = 0;
            float Position = 0.0f;
            bool Before = false;
        };

        MapResult Map(const TweenNode &node, float t)
        {
            MapResult r;
            const float local = t - node.Delay;
            if (local < 0.0f)
            {
                r.Before = true;
                return r;
            }
            const float d = node.CycleLength();
            if (d <= 0.0f)
            {
                r.Position = 0.0f;
                return r;
            }
            const float cycles = node.Repeat < 0 ? Infinite : static_cast<float>(node.Repeat + 1);
            float within;
            if (local >= d * cycles)
            {
                r.Cycle = static_cast<int>(cycles) - 1;
                within = d;
            }
            else
            {
                r.Cycle = static_cast<int>(std::floor(local / d));
                within = local - static_cast<float>(r.Cycle) * d;
            }
            const bool forward = !(node.Yoyo && (r.Cycle % 2) == 1);
            r.Position = forward ? within : d - within;
            return r;
        }

        bool CycleForward(const TweenNode &node, int cycle)
        {
            return !(node.Yoyo && (cycle % 2) == 1);
        }
    }

    float TweenNode::CycleLength() const
    {
        switch (Kind)
        {
        case TweenKind::Group:
            return GroupLength;
        case TweenKind::Callback:
        case TweenKind::Spring:
            return 0.0f;
        default:
            return std::max(Duration, 0.0f);
        }
    }

    float TweenNode::TotalLength() const
    {
        if (Repeat < 0)
            return Infinite;
        return Delay + CycleLength() * static_cast<float>(Repeat + 1);
    }

    TweenSystem::TweenSystem(Scene &scene)
        : m_Scene(scene), m_Serial(s_NextSerial++)
    {
        Systems()[m_Serial] = this;
    }

    TweenSystem::~TweenSystem()
    {
        Systems().erase(m_Serial);
        m_Nodes.clear();
    }

    TweenSystem *TweenSystem::FromSerial(uint64_t serial)
    {
        auto it = Systems().find(serial);
        return it == Systems().end() ? nullptr : it->second;
    }

    bool TweenSystem::PropertyFromName(const std::string &name, TweenProperty &property, int &channels)
    {
        for (const PropertyInfo &info : Properties)
        {
            if (name.size() != std::strlen(info.Name))
                continue;
            bool same = true;
            for (size_t i = 0; i < name.size() && same; ++i)
                same = std::tolower(static_cast<unsigned char>(name[i])) == std::tolower(static_cast<unsigned char>(info.Name[i]));
            if (same)
            {
                property = info.Property;
                channels = info.Channels;
                return true;
            }
        }
        return false;
    }

    const char *TweenSystem::PropertyName(TweenProperty property)
    {
        for (const PropertyInfo &info : Properties)
            if (info.Property == property)
                return info.Name;
        return "?";
    }

    int TweenSystem::ChannelCount(TweenProperty property)
    {
        for (const PropertyInfo &info : Properties)
            if (info.Property == property)
                return info.Channels;
        return 1;
    }

    bool TweenSystem::Read(entt::entity entity, TweenProperty property, float out[4]) const
    {
        const entt::registry &reg = m_Scene.GetRegistry();
        if (!reg.valid(entity))
            return false;
        out[0] = out[1] = out[2] = out[3] = 0.0f;
        const auto *t = reg.try_get<TransformComponent>(entity);
        const auto *sprite = reg.try_get<SpriteComponent>(entity);
        const auto *shape = reg.try_get<RenderComponent>(entity);
        const auto *text = reg.try_get<TextComponent>(entity);
        const auto *vector = reg.try_get<VectorShapeComponent>(entity);

        switch (property)
        {
        case TweenProperty::X:
            if (!t) return false;
            out[0] = t->position.x;
            return true;
        case TweenProperty::Y:
            if (!t) return false;
            out[0] = t->position.y;
            return true;
        case TweenProperty::Position:
            if (!t) return false;
            out[0] = t->position.x;
            out[1] = t->position.y;
            return true;
        case TweenProperty::Rotation:
            if (!t) return false;
            out[0] = t->rotation;
            return true;
        case TweenProperty::ScaleX:
            if (!t) return false;
            out[0] = t->scale.x;
            return true;
        case TweenProperty::ScaleY:
            if (!t) return false;
            out[0] = t->scale.y;
            return true;
        case TweenProperty::Scale:
            if (!t) return false;
            out[0] = t->scale.x;
            out[1] = t->scale.y;
            return true;
        case TweenProperty::Z:
            if (!t) return false;
            out[0] = t->zIndex;
            return true;
        case TweenProperty::Color:
        case TweenProperty::Alpha:
        {
            float c[4];
            if (sprite)
                ReadColor(sprite->tint, c);
            else if (vector)
                ReadColor(vector->fill ? vector->fillColor : vector->strokeColor, c);
            else if (shape)
                ReadColor(shape->color, c);
            else if (text)
                ReadColor(text->Color, c);
            else if (const auto *tilemap = reg.try_get<TilemapComponent>(entity))
                ReadColor(tilemap->tint, c);
            else
                return false;
            if (property == TweenProperty::Alpha)
                out[0] = c[3];
            else
                std::copy(c, c + 4, out);
            return true;
        }
        case TweenProperty::Width:
        case TweenProperty::Height:
        case TweenProperty::Size:
        {
            glm::vec2 size;
            if (sprite)
                size = {sprite->width, sprite->height};
            else if (shape)
                size = {shape->width, shape->height};
            else
                return false;
            if (property == TweenProperty::Width)
                out[0] = size.x;
            else if (property == TweenProperty::Height)
                out[0] = size.y;
            else
            {
                out[0] = size.x;
                out[1] = size.y;
            }
            return true;
        }
        case TweenProperty::Tint:
            if (!sprite) return false;
            ReadColor(sprite->tint, out);
            return true;
        case TweenProperty::ShapeColor:
            if (!shape) return false;
            ReadColor(shape->color, out);
            return true;
        case TweenProperty::TextColor:
            if (!text) return false;
            ReadColor(text->Color, out);
            return true;
        case TweenProperty::FontSize:
            if (!text) return false;
            out[0] = text->Size;
            return true;
        case TweenProperty::LightColor:
        case TweenProperty::LightIntensity:
        case TweenProperty::LightRadius:
        {
            const auto *light = reg.try_get<LightComponent>(entity);
            if (!light) return false;
            if (property == TweenProperty::LightColor)
                ReadColor(light->color, out);
            else
                out[0] = property == TweenProperty::LightIntensity ? light->intensity : light->radius;
            return true;
        }
        case TweenProperty::GlowColor:
        case TweenProperty::GlowIntensity:
        case TweenProperty::GlowSpread:
        {
            const auto *glow = reg.try_get<GlowComponent>(entity);
            if (!glow) return false;
            if (property == TweenProperty::GlowColor)
                ReadColor(glow->color, out);
            else
                out[0] = property == TweenProperty::GlowIntensity ? glow->intensity : glow->spread;
            return true;
        }
        case TweenProperty::Zoom:
        {
            const auto *camera = reg.try_get<CameraComponent>(entity);
            if (!camera) return false;
            out[0] = camera->Camera.GetZoom();
            return true;
        }
        case TweenProperty::ParticleRate:
        {
            const auto *emitter = reg.try_get<ParticleEmitterComponent>(entity);
            if (!emitter) return false;
            out[0] = emitter->effect.Rate;
            return true;
        }
        case TweenProperty::Volume:
        case TweenProperty::Pitch:
        {
            const auto *audio = reg.try_get<AudioSourceComponent>(entity);
            if (!audio) return false;
            out[0] = property == TweenProperty::Volume ? audio->volume : audio->pitch;
            return true;
        }
        case TweenProperty::AnimationSpeed:
        {
            const auto *anim = reg.try_get<AnimatedSpriteComponent>(entity);
            if (!anim) return false;
            out[0] = anim->speed;
            return true;
        }
        case TweenProperty::FillColor:
            if (!vector) return false;
            ReadColor(vector->fillColor, out);
            return true;
        case TweenProperty::StrokeColor:
            if (!vector) return false;
            ReadColor(vector->strokeColor, out);
            return true;
        case TweenProperty::StrokeWidth:
            if (!vector) return false;
            out[0] = vector->strokeWidth;
            return true;
        case TweenProperty::Morph:
            if (!vector) return false;
            out[0] = vector->morph;
            return true;
        case TweenProperty::TrimStart:
            if (!vector) return false;
            out[0] = vector->trimStart;
            return true;
        case TweenProperty::TrimEnd:
            if (!vector) return false;
            out[0] = vector->trimEnd;
            return true;
        case TweenProperty::TrimOffset:
            if (!vector) return false;
            out[0] = vector->trimOffset;
            return true;
        case TweenProperty::DashOffset:
            if (!vector) return false;
            out[0] = vector->dashOffset;
            return true;
        case TweenProperty::Count:
            break;
        }
        return false;
    }

    void TweenSystem::Write(entt::entity entity, TweenProperty property, const float v[4])
    {
        entt::registry &reg = m_Scene.GetRegistry();
        if (!reg.valid(entity))
            return;
        auto *t = reg.try_get<TransformComponent>(entity);
        auto *sprite = reg.try_get<SpriteComponent>(entity);
        auto *shape = reg.try_get<RenderComponent>(entity);
        auto *text = reg.try_get<TextComponent>(entity);
        auto *vector = reg.try_get<VectorShapeComponent>(entity);

        auto syncBody = [&]()
        {
            if (auto *physics = reg.try_get<PhysicsComponent>(entity); physics && physics->body && t)
            {
                physics->body->position = {t->position.x, t->position.y};
                physics->body->orientation = glm::radians(t->rotation);
                physics->body->calculateDerivativeData();
            }
        };

        switch (property)
        {
        case TweenProperty::X:
            if (t) { t->position.x = v[0]; t->dirty = true; syncBody(); }
            break;
        case TweenProperty::Y:
            if (t) { t->position.y = v[0]; t->dirty = true; syncBody(); }
            break;
        case TweenProperty::Position:
            if (t) { t->position = {v[0], v[1]}; t->dirty = true; syncBody(); }
            break;
        case TweenProperty::Rotation:
            if (t) { t->rotation = v[0]; t->dirty = true; syncBody(); }
            break;
        case TweenProperty::ScaleX:
            if (t) { t->scale.x = v[0]; t->dirty = true; }
            break;
        case TweenProperty::ScaleY:
            if (t) { t->scale.y = v[0]; t->dirty = true; }
            break;
        case TweenProperty::Scale:
            if (t) { t->scale = {v[0], v[1]}; t->dirty = true; }
            break;
        case TweenProperty::Z:
            if (t) t->zIndex = v[0];
            break;
        case TweenProperty::Color:
            if (sprite)
                WriteColor(sprite->tint, v);
            else if (vector)
                WriteColor(vector->fill ? vector->fillColor : vector->strokeColor, v);
            else if (shape)
                WriteColor(shape->color, v);
            else if (text)
                WriteColor(text->Color, v);
            else if (auto *tilemap = reg.try_get<TilemapComponent>(entity))
                WriteColor(tilemap->tint, v);
            break;
        case TweenProperty::Alpha:
        {
            const Uint8 a = Byte(v[0]);
            if (sprite)
                sprite->tint.a = a;
            else if (vector)
            {
                vector->fillColor.a = a;
                vector->strokeColor.a = a;
            }
            else if (shape)
                shape->color.a = a;
            else if (text)
                text->Color.a = a;
            else if (auto *tilemap = reg.try_get<TilemapComponent>(entity))
                tilemap->tint.a = a;
            break;
        }
        case TweenProperty::Width:
            if (sprite) sprite->width = v[0];
            else if (shape) shape->width = v[0];
            break;
        case TweenProperty::Height:
            if (sprite) sprite->height = v[0];
            else if (shape) shape->height = v[0];
            break;
        case TweenProperty::Size:
            if (sprite) { sprite->width = v[0]; sprite->height = v[1]; }
            else if (shape) { shape->width = v[0]; shape->height = v[1]; }
            break;
        case TweenProperty::Tint:
            if (sprite) WriteColor(sprite->tint, v);
            break;
        case TweenProperty::ShapeColor:
            if (shape) WriteColor(shape->color, v);
            break;
        case TweenProperty::TextColor:
            if (text) WriteColor(text->Color, v);
            break;
        case TweenProperty::FontSize:
            if (text) text->Size = std::max(1.0f, v[0]);
            break;
        case TweenProperty::LightColor:
            if (auto *light = reg.try_get<LightComponent>(entity)) WriteColor(light->color, v);
            break;
        case TweenProperty::LightIntensity:
            if (auto *light = reg.try_get<LightComponent>(entity)) light->intensity = std::max(0.0f, v[0]);
            break;
        case TweenProperty::LightRadius:
            if (auto *light = reg.try_get<LightComponent>(entity)) light->radius = std::max(0.0f, v[0]);
            break;
        case TweenProperty::GlowColor:
            if (auto *glow = reg.try_get<GlowComponent>(entity)) WriteColor(glow->color, v);
            break;
        case TweenProperty::GlowIntensity:
            if (auto *glow = reg.try_get<GlowComponent>(entity)) glow->intensity = std::max(0.0f, v[0]);
            break;
        case TweenProperty::GlowSpread:
            if (auto *glow = reg.try_get<GlowComponent>(entity)) glow->spread = std::max(0.0f, v[0]);
            break;
        case TweenProperty::Zoom:
            if (auto *camera = reg.try_get<CameraComponent>(entity)) camera->Camera.SetZoom(std::max(0.01f, v[0]));
            break;
        case TweenProperty::ParticleRate:
            if (auto *emitter = reg.try_get<ParticleEmitterComponent>(entity)) emitter->effect.Rate = std::max(0.0f, v[0]);
            break;
        case TweenProperty::Volume:
            if (auto *audio = reg.try_get<AudioSourceComponent>(entity)) audio->volume = std::max(0.0f, v[0]);
            break;
        case TweenProperty::Pitch:
            if (auto *audio = reg.try_get<AudioSourceComponent>(entity)) audio->pitch = std::max(0.01f, v[0]);
            break;
        case TweenProperty::AnimationSpeed:
            if (auto *anim = reg.try_get<AnimatedSpriteComponent>(entity)) anim->speed = std::max(0.0f, v[0]);
            break;
        case TweenProperty::FillColor:
            if (vector) WriteColor(vector->fillColor, v);
            break;
        case TweenProperty::StrokeColor:
            if (vector) WriteColor(vector->strokeColor, v);
            break;
        case TweenProperty::StrokeWidth:
            if (vector) vector->strokeWidth = std::max(0.0f, v[0]);
            break;
        case TweenProperty::Morph:
            if (vector) vector->morph = v[0];
            break;
        case TweenProperty::TrimStart:
            if (vector) vector->trimStart = v[0];
            break;
        case TweenProperty::TrimEnd:
            if (vector) vector->trimEnd = v[0];
            break;
        case TweenProperty::TrimOffset:
            if (vector) vector->trimOffset = v[0];
            break;
        case TweenProperty::DashOffset:
            if (vector) vector->dashOffset = v[0];
            break;
        case TweenProperty::Count:
            break;
        }
    }

    uint32_t TweenSystem::Add(std::unique_ptr<TweenNode> node)
    {
        const uint32_t id = m_NextId++;
        if (m_NextId == 0)
            m_NextId = 1;
        node->Id = id;
        m_Order.push_back(id);
        m_Nodes[id] = std::move(node);
        return id;
    }

    uint32_t TweenSystem::To(entt::entity target, std::vector<TweenTrack> tracks, float duration, const Curve &curve)
    {
        auto node = std::make_unique<TweenNode>();
        node->Kind = TweenKind::Property;
        node->Target = target;
        node->Tracks = std::move(tracks);
        node->Duration = std::max(duration, 0.0f);
        node->Ease = curve;
        return Add(std::move(node));
    }

    uint32_t TweenSystem::Value(float from, float to, float duration, const Curve &curve, std::function<void(float)> onValue)
    {
        auto node = std::make_unique<TweenNode>();
        node->Kind = TweenKind::Value;
        node->ValueFrom = from;
        node->ValueTo = to;
        node->Duration = std::max(duration, 0.0f);
        node->Ease = curve;
        node->OnValue = std::move(onValue);
        return Add(std::move(node));
    }

    uint32_t TweenSystem::Motion(entt::entity target, std::vector<glm::vec2> points, bool closed, float duration, const Curve &curve, bool orient, bool relative)
    {
        auto node = std::make_unique<TweenNode>();
        node->Kind = TweenKind::Motion;
        node->Target = target;
        node->Points = std::move(points);
        node->Closed = closed;
        node->Duration = std::max(duration, 0.0f);
        node->Ease = curve;
        node->Orient = orient;
        node->RelativeMotion = relative;
        return Add(std::move(node));
    }

    uint32_t TweenSystem::Spring(entt::entity target, std::vector<TweenTrack> tracks, const SpringParams &params)
    {
        auto node = std::make_unique<TweenNode>();
        node->Kind = TweenKind::Spring;
        node->Target = target;
        node->Tracks = std::move(tracks);
        node->Spring = params;
        node->Captured = true;
        const uint32_t id = Add(std::move(node));
        Overwrite(*m_Nodes[id]);
        return id;
    }

    void TweenSystem::RetargetSpring(uint32_t id, const std::vector<TweenTrack> &tracks, const SpringParams *params)
    {
        TweenNode *node = Get(id);
        if (!node || node->Kind != TweenKind::Spring)
            return;
        if (params)
            node->Spring = *params;
        for (const TweenTrack &incoming : tracks)
        {
            auto it = std::find_if(node->Tracks.begin(), node->Tracks.end(), [&](const TweenTrack &t)
                                   { return t.Property == incoming.Property; });
            if (it != node->Tracks.end())
            {
                for (int c = 0; c < incoming.Channels; ++c)
                    if (!std::isnan(incoming.To[c]))
                        it->To[c] = incoming.To[c];
            }
            else
                node->Tracks.push_back(incoming);
        }
        node->Finished = false;
        node->Completed = false;
        node->Playing = true;
        node->Dead = false;
        Overwrite(*node);
    }

    uint32_t TweenSystem::Group(bool sequence, const std::vector<uint32_t> &children, std::string *error)
    {
        auto node = std::make_unique<TweenNode>();
        node->Kind = TweenKind::Group;
        node->Sequence = sequence;
        float cursor = 0.0f;
        float length = 0.0f;
        for (uint32_t id : children)
        {
            TweenNode *child = Get(id);
            if (!child || child->Owned || child->Kind == TweenKind::Spring)
            {
                if (error)
                    *error = !child ? "unknown tween in group" : child->Owned ? "tween already belongs to a group" : "springs cannot be grouped";
                return 0;
            }
            const float total = child->TotalLength();
            if (std::isinf(total) && (sequence || children.size() > 1))
            {
                if (error)
                    *error = "a tween that repeats forever cannot be inside a group";
                return 0;
            }
        }
        for (uint32_t id : children)
        {
            TweenNode *child = Get(id);
            child->Owned = true;
            child->Time = 0.0f;
            child->Finished = false;
            child->Playing = true;
            m_Order.erase(std::remove(m_Order.begin(), m_Order.end(), id), m_Order.end());
            const float total = child->TotalLength();
            node->Children.push_back(id);
            node->Offsets.push_back(sequence ? cursor : 0.0f);
            if (sequence)
                cursor += total;
            length = sequence ? cursor : std::max(length, total);
        }
        node->GroupLength = length;
        node->Duration = length;
        return Add(std::move(node));
    }

    uint32_t TweenSystem::Delay(float seconds)
    {
        auto node = std::make_unique<TweenNode>();
        node->Kind = TweenKind::Value;
        node->Duration = std::max(seconds, 0.0f);
        return Add(std::move(node));
    }

    uint32_t TweenSystem::Call(std::function<void()> callback)
    {
        auto node = std::make_unique<TweenNode>();
        node->Kind = TweenKind::Callback;
        node->Duration = 0.0f;
        node->OnComplete = std::move(callback);
        return Add(std::move(node));
    }

    TweenNode *TweenSystem::Get(uint32_t id)
    {
        auto it = m_Nodes.find(id);
        return it == m_Nodes.end() ? nullptr : it->second.get();
    }

    const TweenNode *TweenSystem::Get(uint32_t id) const
    {
        auto it = m_Nodes.find(id);
        return it == m_Nodes.end() ? nullptr : it->second.get();
    }

    TweenNode *TweenSystem::FindSpring(entt::entity target)
    {
        for (auto &[id, node] : m_Nodes)
            if (node->Kind == TweenKind::Spring && node->Target == target && !node->Dead)
                return node.get();
        return nullptr;
    }

    bool TweenSystem::TargetAlive(const TweenNode &node) const
    {
        if (node.Target == entt::null)
            return node.Kind == TweenKind::Value || node.Kind == TweenKind::Group || node.Kind == TweenKind::Callback;
        return m_Scene.GetRegistry().valid(node.Target);
    }

    void TweenSystem::Capture(TweenNode &node)
    {
        node.Captured = true;
        if (node.Kind == TweenKind::Property)
        {
            for (TweenTrack &track : node.Tracks)
            {
                float current[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                const bool known = Read(node.Target, track.Property, current);
                if (!track.ExplicitFrom)
                    std::copy(current, current + 4, track.From);
                else if (track.RelativeFrom)
                {
                    for (int c = 0; c < 4; ++c)
                        if (!std::isnan(track.From[c]))
                            track.From[c] += current[c];
                    track.RelativeFrom = false;
                }
                for (int c = 0; c < 4; ++c)
                {
                    if (std::isnan(track.From[c]))
                        track.From[c] = current[c];
                    if (std::isnan(track.To[c]))
                        track.To[c] = track.Relative ? 0.0f : current[c];
                }
                if (track.Relative)
                {
                    for (int c = 0; c < track.Channels; ++c)
                        track.To[c] = track.From[c] + track.To[c];
                    track.Relative = false;
                }
                if (!known)
                    track.Channels = 0;
            }
        }
        else if (node.Kind == TweenKind::Motion)
        {
            float current[4];
            if (node.RelativeMotion && !node.Points.empty() && Read(node.Target, TweenProperty::Position, current))
                node.MotionOffset = glm::vec2(current[0], current[1]) - node.Points.front();
        }
        if (!node.Owned)
            Overwrite(node);
        if (node.OnStart)
            node.OnStart();
    }

    void TweenSystem::Overwrite(TweenNode &node)
    {
        if (node.Target == entt::null)
            return;
        std::vector<TweenProperty> mine;
        if (node.Kind == TweenKind::Motion)
        {
            mine.push_back(TweenProperty::Position);
            if (node.Orient)
                mine.push_back(TweenProperty::Rotation);
        }
        else
            for (const TweenTrack &track : node.Tracks)
                mine.push_back(track.Property);

        for (auto &[id, other] : m_Nodes)
        {
            if (other.get() == &node || other->Owned || other->Dead || other->Target != node.Target)
                continue;
            if (other->Kind == TweenKind::Motion)
            {
                const bool clash = std::any_of(mine.begin(), mine.end(), [](TweenProperty p)
                                               { return Overlaps(p, TweenProperty::Position); });
                if (clash && (other->Captured && !other->Finished))
                    MarkDead(*other);
                continue;
            }
            if (other->Kind != TweenKind::Property && other->Kind != TweenKind::Spring)
                continue;
            if (other->Kind == TweenKind::Property && (!other->Captured || other->Finished))
                continue;
            auto &tracks = other->Tracks;
            tracks.erase(std::remove_if(tracks.begin(), tracks.end(), [&](const TweenTrack &track)
                                        { return std::any_of(mine.begin(), mine.end(), [&](TweenProperty p)
                                                             { return Overlaps(p, track.Property); }); }),
                         tracks.end());
            if (tracks.empty())
                MarkDead(*other);
        }
    }

    void TweenSystem::MarkDead(TweenNode &node)
    {
        node.Dead = true;
        node.Playing = false;
        for (uint32_t child : node.Children)
            if (TweenNode *c = Get(child))
                MarkDead(*c);
    }

    void TweenSystem::Apply(TweenNode &node, float eased)
    {
        switch (node.Kind)
        {
        case TweenKind::Property:
            for (const TweenTrack &track : node.Tracks)
            {
                if (track.Channels <= 0)
                    continue;
                float value[4];
                Read(node.Target, track.Property, value);
                for (int c = 0; c < track.Channels; ++c)
                    value[c] = track.From[c] + (track.To[c] - track.From[c]) * eased;
                Write(node.Target, track.Property, value);
            }
            break;
        case TweenKind::Value:
            if (node.OnValue)
                node.OnValue(node.ValueFrom + (node.ValueTo - node.ValueFrom) * eased);
            break;
        case TweenKind::Motion:
        {
            if (node.Points.empty())
                break;
            const float length = PolylineLength(node.Points, node.Closed);
            glm::vec2 point, tangent;
            const float distance = node.Closed ? eased * length : std::clamp(eased, 0.0f, 1.0f) * length;
            if (!PointAtDistance(node.Points, node.Closed, distance, point, tangent))
                break;
            point += node.MotionOffset;
            float value[4] = {point.x, point.y, 0.0f, 0.0f};
            Write(node.Target, TweenProperty::Position, value);
            if (node.Orient)
            {
                float angle[4] = {glm::degrees(std::atan2(tangent.y, tangent.x)) + node.OrientOffset, 0.0f, 0.0f, 0.0f};
                Write(node.Target, TweenProperty::Rotation, angle);
            }
            break;
        }
        default:
            break;
        }
    }

    void TweenSystem::Render(TweenNode &node, float prev, float now)
    {
        if (node.Dead)
            return;
        const float total = node.TotalLength();
        if (std::max(prev, now) < 0.0f || std::min(prev, now) > total)
            return;
        const float clampedPrev = std::clamp(prev, 0.0f, total);
        const float clampedNow = std::clamp(now, 0.0f, total);

        if (node.Kind == TweenKind::Callback)
        {
            if (now < total)
                node.Completed = false;
            else if (!node.Completed)
            {
                node.Completed = true;
                if (node.OnComplete)
                    node.OnComplete();
            }
            return;
        }

        const MapResult current = Map(node, clampedNow);
        const float d = node.CycleLength();

        if (node.Kind == TweenKind::Group)
        {
            if (!current.Before && !node.Captured)
            {
                node.Captured = true;
                if (node.OnStart)
                    node.OnStart();
            }
            const MapResult before = Map(node, clampedPrev);
            auto eased = [&](float pos)
            {
                if (node.Ease.GetKind() == CurveKind::Linear || d <= 0.0f)
                    return pos;
                return node.Ease.Evaluate(pos / d) * d;
            };
            const float startPrev = before.Before ? (now >= prev ? 0.0f : d) : before.Position;
            if (before.Cycle == current.Cycle || before.Before || current.Before)
            {
                if (!current.Before)
                    RenderGroupRange(node, eased(startPrev), eased(current.Position));
                else if (!before.Before)
                    RenderGroupRange(node, eased(startPrev), 0.0f);
            }
            else if (now > prev)
            {
                RenderGroupRange(node, eased(before.Position), eased(CycleForward(node, before.Cycle) ? d : 0.0f));
                if (node.OnRepeat)
                    node.OnRepeat();
                RenderGroupRange(node, eased(CycleForward(node, current.Cycle) ? 0.0f : d), eased(current.Position));
            }
            else
            {
                RenderGroupRange(node, eased(before.Position), eased(CycleForward(node, before.Cycle) ? 0.0f : d));
                RenderGroupRange(node, eased(CycleForward(node, current.Cycle) ? d : 0.0f), eased(current.Position));
            }
            if (node.OnUpdate)
                node.OnUpdate(d > 0.0f ? current.Position / d : 1.0f);
            FireComplete(node, now, total);
            return;
        }

        if (current.Before && !node.Captured)
            return;
        if (!node.Captured)
            Capture(node);
        if (node.Dead)
            return;

        const float p = current.Before ? 0.0f : (d > 0.0f ? current.Position / d : 1.0f);
        const float e = node.Ease.Evaluate(p);
        Apply(node, e);

        if (node.OnUpdate)
            node.OnUpdate(e);
        if (!current.Before && node.OnRepeat)
        {
            const MapResult before = Map(node, clampedPrev);
            if (!before.Before && before.Cycle != current.Cycle)
                node.OnRepeat();
        }
        FireComplete(node, now, total);
    }

    void TweenSystem::FireComplete(TweenNode &node, float now, float total)
    {
        if (now < total)
        {
            node.Completed = false;
            return;
        }
        if (node.Completed)
            return;
        node.Completed = true;
        if (node.OnComplete)
            node.OnComplete();
    }

    void TweenSystem::RenderGroupRange(TweenNode &node, float a, float b)
    {
        const std::vector<uint32_t> children = node.Children;
        const std::vector<float> offsets = node.Offsets;
        if (b >= a)
        {
            for (size_t i = 0; i < children.size(); ++i)
                if (TweenNode *child = Get(children[i]))
                    Render(*child, a - offsets[i], b - offsets[i]);
        }
        else
        {
            for (size_t i = children.size(); i-- > 0;)
                if (TweenNode *child = Get(children[i]))
                    Render(*child, a - offsets[i], b - offsets[i]);
        }
    }

    void TweenSystem::UpdateSpring(TweenNode &node, float dt)
    {
        if (!node.Playing || node.Finished)
            return;
        const float k = std::max(node.Spring.Stiffness, 0.0f);
        const float c = std::max(node.Spring.Damping, 0.0f);
        const float m = std::max(node.Spring.Mass, 0.001f);
        const int steps = std::clamp(static_cast<int>(std::ceil(dt * node.Speed / (1.0f / 240.0f))), 1, 32);
        const float h = dt * node.Speed / static_cast<float>(steps);
        bool resting = true;
        for (TweenTrack &track : node.Tracks)
        {
            float value[4];
            if (!Read(node.Target, track.Property, value))
                continue;
            for (int ch = 0; ch < track.Channels; ++ch)
            {
                float x = value[ch];
                float v = track.Velocity[ch];
                for (int s = 0; s < steps; ++s)
                {
                    const float a = (-k * (x - track.To[ch]) - c * v) / m;
                    v += a * h;
                    x += v * h;
                }
                const float scale = std::max(1.0f, std::abs(track.To[ch]) * 0.001f);
                if (std::abs(x - track.To[ch]) > node.Spring.RestThreshold * scale || std::abs(v) > node.Spring.RestThreshold * 10.0f * scale)
                    resting = false;
                value[ch] = x;
                track.Velocity[ch] = v;
            }
            Write(node.Target, track.Property, value);
        }
        if (node.OnUpdate)
            node.OnUpdate(0.0f);
        if (resting)
        {
            for (TweenTrack &track : node.Tracks)
            {
                float value[4];
                if (!Read(node.Target, track.Property, value))
                    continue;
                for (int ch = 0; ch < track.Channels; ++ch)
                {
                    value[ch] = track.To[ch];
                    track.Velocity[ch] = 0.0f;
                }
                Write(node.Target, track.Property, value);
            }
            node.Finished = true;
            if (node.OnComplete)
                node.OnComplete();
        }
    }

    void TweenSystem::Update(float dt)
    {
        if (m_Order.empty())
            return;
        m_Updating = true;
        const std::vector<uint32_t> order = m_Order;
        for (uint32_t id : order)
        {
            if (m_ClearRequested)
                break;
            TweenNode *node = Get(id);
            if (!node || node->Dead || node->Owned)
                continue;
            if (!TargetAlive(*node))
            {
                MarkDead(*node);
                continue;
            }
            if (node->Kind == TweenKind::Spring)
            {
                UpdateSpring(*node, dt);
                if (node->Finished && node->Refs <= 0)
                    MarkDead(*node);
                continue;
            }
            if (!node->Playing || node->Finished)
            {
                if (node->Finished && node->Refs <= 0)
                    MarkDead(*node);
                continue;
            }

            const float prev = node->Time;
            const float total = node->TotalLength();
            const float step = dt * node->Speed;
            if (!node->Reversed)
            {
                float now = prev + step;
                if (now >= total)
                {
                    now = total;
                    node->Finished = true;
                }
                node->Time = now;
                Render(*node, prev, now);
            }
            else
            {
                float now = prev - step;
                bool atStart = false;
                if (now <= 0.0f)
                {
                    now = 0.0f;
                    node->Finished = true;
                    atStart = true;
                }
                node->Time = now;
                Render(*node, prev, now);
                if (atStart && node->Finished && node->OnComplete)
                    node->OnComplete();
            }

            const float d = node->CycleLength();
            if (node->Repeat < 0 && d > 0.0f && node->Time - node->Delay > 2000.0f * d)
                node->Time -= 1000.0f * d * 2.0f;

            if (node->Finished && node->Refs <= 0)
                MarkDead(*node);
        }
        m_Updating = false;

        if (m_ClearRequested)
        {
            m_ClearRequested = false;
            Clear();
            return;
        }

        std::vector<uint32_t> dead;
        for (auto &[id, node] : m_Nodes)
            if (node->Dead && node->Refs <= 0)
                dead.push_back(id);
        for (uint32_t id : dead)
        {
            m_Nodes.erase(id);
            m_Order.erase(std::remove(m_Order.begin(), m_Order.end(), id), m_Order.end());
        }
    }

    void TweenSystem::Play(uint32_t id)
    {
        TweenNode *node = Get(id);
        if (!node || node->Dead)
            return;
        if (node->Finished && node->Kind != TweenKind::Spring)
        {
            Restart(id);
            return;
        }
        node->Playing = true;
    }

    void TweenSystem::Pause(uint32_t id)
    {
        if (TweenNode *node = Get(id))
            node->Playing = false;
    }

    void TweenSystem::Restart(uint32_t id)
    {
        TweenNode *node = Get(id);
        if (!node || node->Dead || node->Owned)
            return;
        const float prev = node->Time;
        node->Time = 0.0f;
        node->Finished = false;
        node->Completed = false;
        node->Playing = true;
        node->Reversed = false;
        if (node->Kind != TweenKind::Spring && node->Captured)
            Render(*node, prev, 0.0f);
    }

    void TweenSystem::Reverse(uint32_t id)
    {
        TweenNode *node = Get(id);
        if (!node || node->Dead || node->Owned)
            return;
        if (std::isinf(node->TotalLength()))
            return;
        node->Reversed = true;
        node->Finished = node->Time <= 0.0f;
        node->Playing = true;
    }

    void TweenSystem::Forward(uint32_t id)
    {
        TweenNode *node = Get(id);
        if (!node || node->Dead || node->Owned)
            return;
        node->Reversed = false;
        node->Finished = node->Time >= node->TotalLength();
        node->Playing = true;
    }

    void TweenSystem::Seek(uint32_t id, float progress)
    {
        TweenNode *node = Get(id);
        if (!node || node->Dead || node->Owned || node->Kind == TweenKind::Spring)
            return;
        const float total = node->TotalLength();
        const float span = std::isinf(total) ? node->Delay + node->CycleLength() : total;
        const float prev = node->Time;
        node->Time = std::clamp(progress, 0.0f, 1.0f) * span;
        node->Finished = !std::isinf(total) && (node->Reversed ? node->Time <= 0.0f : node->Time >= total);
        Render(*node, prev, node->Time);
    }

    void TweenSystem::ImmediateRender(uint32_t id)
    {
        TweenNode *node = Get(id);
        if (!node || node->Dead || node->Kind != TweenKind::Property)
            return;
        if (!node->Captured)
            Capture(*node);
        Apply(*node, node->Ease.Evaluate(0.0f));
    }

    void TweenSystem::Kill(uint32_t id)
    {
        if (TweenNode *node = Get(id))
            MarkDead(*node);
    }

    float TweenSystem::Progress(uint32_t id) const
    {
        const TweenNode *node = Get(id);
        if (!node)
            return 1.0f;
        if (node->Kind == TweenKind::Spring)
            return node->Finished ? 1.0f : 0.0f;
        const float total = node->TotalLength();
        if (std::isinf(total))
        {
            const float d = node->CycleLength();
            return d > 0.0f ? std::fmod(std::max(node->Time - node->Delay, 0.0f), d) / d : 0.0f;
        }
        return total > 0.0f ? std::clamp(node->Time / total, 0.0f, 1.0f) : 1.0f;
    }

    bool TweenSystem::IsRunning(uint32_t id) const
    {
        const TweenNode *node = Get(id);
        return node && !node->Dead && node->Playing && !node->Finished;
    }

    void TweenSystem::KillTarget(entt::entity target, int property)
    {
        for (auto &[id, node] : m_Nodes)
        {
            if (node->Target != target || node->Dead || node->Owned)
                continue;
            if (property < 0 || node->Kind == TweenKind::Motion)
            {
                MarkDead(*node);
                continue;
            }
            auto &tracks = node->Tracks;
            tracks.erase(std::remove_if(tracks.begin(), tracks.end(), [&](const TweenTrack &track)
                                        { return Overlaps(track.Property, static_cast<TweenProperty>(property)); }),
                         tracks.end());
            if (tracks.empty())
                MarkDead(*node);
        }
    }

    bool TweenSystem::IsTweening(entt::entity target) const
    {
        for (const auto &[id, node] : m_Nodes)
            if (node->Target == target && !node->Dead && !node->Finished && node->Playing)
                return true;
        return false;
    }

    void TweenSystem::Clear()
    {
        if (m_Updating)
        {
            for (auto &[id, node] : m_Nodes)
                MarkDead(*node);
            return;
        }
        for (auto &[id, node] : m_Nodes)
            node->Dead = true;
        for (auto it = m_Nodes.begin(); it != m_Nodes.end();)
        {
            if (it->second->Refs > 0)
            {
                it->second->Target = entt::null;
                it->second->OnStart = nullptr;
                it->second->OnUpdate = nullptr;
                it->second->OnComplete = nullptr;
                it->second->OnRepeat = nullptr;
                it->second->OnValue = nullptr;
                ++it;
            }
            else
                it = m_Nodes.erase(it);
        }
        m_Order.clear();
    }

    void TweenSystem::AddRef(uint32_t id)
    {
        if (TweenNode *node = Get(id))
            ++node->Refs;
    }

    void TweenSystem::Release(uint32_t id)
    {
        TweenNode *node = Get(id);
        if (!node)
            return;
        --node->Refs;
        if (node->Refs <= 0 && (node->Dead || (node->Finished && !node->Owned)))
        {
            if (m_Updating)
            {
                node->Dead = true;
                return;
            }
            std::vector<uint32_t> stack{id};
            while (!stack.empty())
            {
                const uint32_t current = stack.back();
                stack.pop_back();
                TweenNode *n = Get(current);
                if (!n)
                    continue;
                for (uint32_t child : n->Children)
                    stack.push_back(child);
                m_Nodes.erase(current);
                m_Order.erase(std::remove(m_Order.begin(), m_Order.end(), current), m_Order.end());
            }
        }
    }
}
