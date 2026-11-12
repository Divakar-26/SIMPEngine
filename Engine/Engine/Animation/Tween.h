#pragma once

#include <Engine/Animation/Curves.h>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace SIMPEngine
{
    class Scene;

    enum class TweenProperty : uint8_t
    {
        X,
        Y,
        Position,
        Rotation,
        ScaleX,
        ScaleY,
        Scale,
        Z,
        Color,
        Alpha,
        Width,
        Height,
        Size,
        Tint,
        ShapeColor,
        TextColor,
        FontSize,
        LightColor,
        LightIntensity,
        LightRadius,
        GlowColor,
        GlowIntensity,
        GlowSpread,
        Zoom,
        ParticleRate,
        Volume,
        Pitch,
        AnimationSpeed,
        FillColor,
        StrokeColor,
        StrokeWidth,
        Morph,
        TrimStart,
        TrimEnd,
        TrimOffset,
        DashOffset,
        ShaderProgress,
        ShaderUniform,
        Count
    };

    struct TweenTrack
    {
        TweenProperty Property = TweenProperty::X;
        int Channels = 1;
        float From[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float To[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        bool ExplicitFrom = false;
        bool Relative = false;
        bool RelativeFrom = false;
        float Velocity[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        std::string Uniform;
    };

    enum class TweenKind : uint8_t
    {
        Property,
        Value,
        Motion,
        Group,
        Callback,
        Spring
    };

    struct SpringParams
    {
        float Stiffness = 180.0f;
        float Damping = 14.0f;
        float Mass = 1.0f;
        float RestThreshold = 0.01f;
    };

    struct TweenNode
    {
        uint32_t Id = 0;
        TweenKind Kind = TweenKind::Property;
        entt::entity Target = entt::null;
        std::vector<TweenTrack> Tracks;
        float Duration = 0.5f;
        float Delay = 0.0f;
        Curve Ease;
        int Repeat = 0;
        bool Yoyo = false;
        float Speed = 1.0f;

        float Time = 0.0f;
        bool Playing = true;
        bool Reversed = false;
        bool Finished = false;
        bool Dead = false;
        bool Captured = false;
        bool Completed = false;
        bool Owned = false;
        int Refs = 0;

        bool Sequence = false;
        std::vector<uint32_t> Children;
        std::vector<float> Offsets;
        float GroupLength = 0.0f;

        float ValueFrom = 0.0f;
        float ValueTo = 1.0f;

        std::vector<glm::vec2> Points;
        bool Closed = false;
        bool Orient = false;
        float OrientOffset = 0.0f;
        bool RelativeMotion = false;
        glm::vec2 MotionOffset{0.0f};

        SpringParams Spring;

        std::function<void()> OnStart;
        std::function<void(float)> OnUpdate;
        std::function<void()> OnComplete;
        std::function<void()> OnRepeat;
        std::function<void(float)> OnValue;

        float CycleLength() const;
        float TotalLength() const;
    };

    class TweenSystem
    {
    public:
        explicit TweenSystem(Scene &scene);
        ~TweenSystem();

        TweenSystem(const TweenSystem &) = delete;
        TweenSystem &operator=(const TweenSystem &) = delete;

        uint32_t To(entt::entity target, std::vector<TweenTrack> tracks, float duration, const Curve &curve);
        uint32_t Value(float from, float to, float duration, const Curve &curve, std::function<void(float)> onValue);
        uint32_t Motion(entt::entity target, std::vector<glm::vec2> points, bool closed, float duration, const Curve &curve, bool orient, bool relative);
        uint32_t Spring(entt::entity target, std::vector<TweenTrack> tracks, const SpringParams &params);
        void RetargetSpring(uint32_t id, const std::vector<TweenTrack> &tracks, const SpringParams *params);
        uint32_t Group(bool sequence, const std::vector<uint32_t> &children, std::string *error = nullptr);
        uint32_t Delay(float seconds);
        uint32_t Call(std::function<void()> callback);

        TweenNode *Get(uint32_t id);
        const TweenNode *Get(uint32_t id) const;
        TweenNode *FindSpring(entt::entity target);

        void Play(uint32_t id);
        void Pause(uint32_t id);
        void Restart(uint32_t id);
        void Reverse(uint32_t id);
        void Forward(uint32_t id);
        void Seek(uint32_t id, float progress);
        void ImmediateRender(uint32_t id);
        void Kill(uint32_t id);
        float Progress(uint32_t id) const;
        bool IsRunning(uint32_t id) const;

        void KillTarget(entt::entity target, int property = -1, const std::string &uniformName = std::string());
        bool IsTweening(entt::entity target) const;
        void Clear();
        void Update(float dt);
        size_t Count() const { return m_Nodes.size(); }

        void AddRef(uint32_t id);
        void Release(uint32_t id);
        uint64_t Serial() const { return m_Serial; }
        static TweenSystem *FromSerial(uint64_t serial);
        Scene &GetScene() { return m_Scene; }

        static bool PropertyFromName(const std::string &name, TweenProperty &property, int &channels);
        static const char *PropertyName(TweenProperty property);
        static int ChannelCount(TweenProperty property);
        bool Read(entt::entity entity, TweenProperty property, float out[4], const std::string *uniform = nullptr) const;
        void Write(entt::entity entity, TweenProperty property, const float value[4], const std::string *uniform = nullptr);

    private:
        uint32_t Add(std::unique_ptr<TweenNode> node);
        void Render(TweenNode &node, float prev, float now);
        void RenderGroupRange(TweenNode &node, float a, float b);
        void FireComplete(TweenNode &node, float now, float total);
        void Apply(TweenNode &node, float eased);
        void Capture(TweenNode &node);
        void Overwrite(TweenNode &node);
        void UpdateSpring(TweenNode &node, float dt);
        void MarkDead(TweenNode &node);
        bool TargetAlive(const TweenNode &node) const;

        Scene &m_Scene;
        uint64_t m_Serial = 0;
        uint32_t m_NextId = 1;
        std::unordered_map<uint32_t, std::unique_ptr<TweenNode>> m_Nodes;
        std::vector<uint32_t> m_Order;
        bool m_Updating = false;
        bool m_ClearRequested = false;
    };
}
