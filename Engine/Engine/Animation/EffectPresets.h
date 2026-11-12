#pragma once

#include <Engine/Animation/Tween.h>

#include <entt/entt.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace SIMPEngine
{
    struct EffectTrack
    {
        TweenProperty Property = TweenProperty::Alpha;
        int Channels = 1;
        float From[2] = {0.0f, 0.0f};
        float To[2] = {0.0f, 0.0f};
        bool HasFrom = false;
        bool RelativeFrom = false;
        bool KeepTo = false;
        bool Relative = false;
        bool Distance = false;
    };

    struct EffectSegment
    {
        std::vector<EffectTrack> Tracks;
        float Length = 1.0f;
        std::string Curve = "easeOut";
    };

    struct EffectLane
    {
        float Delay = 0.0f;
        std::vector<EffectSegment> Segments;
    };

    struct EffectPreset
    {
        std::string Name;
        std::string Label;
        std::string Category;
        std::string Description;
        float Duration = 0.6f;
        bool Loop = false;
        std::vector<EffectLane> Lanes;
    };

    struct EffectOptions
    {
        float Duration = 0.0f;
        float Delay = 0.0f;
        float Distance = 1.0f;
        float Amount = 1.0f;
        float Speed = 1.0f;
        int Loop = -2;
    };

    namespace EffectPresets
    {
        const std::vector<EffectPreset> &All();
        const EffectPreset *Find(const std::string &name);
        std::vector<std::string> Categories();

        uint32_t Play(TweenSystem &system, entt::entity target, const EffectPreset &preset, const EffectOptions &options = {});

        std::string OneLiner(const EffectPreset &preset);
        std::string PlayCall(const EffectPreset &preset, const EffectOptions &options = {});
        std::string ExpandedLua(const EffectPreset &preset, const EffectOptions &options = {});
    }
}
