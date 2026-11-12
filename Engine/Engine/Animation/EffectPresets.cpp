#include <Engine/Animation/EffectPresets.h>

#include <Engine/Animation/Curves.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace SIMPEngine
{
    namespace
    {
        constexpr float Keep = std::numeric_limits<float>::quiet_NaN();

        EffectTrack FromAlpha(float v)
        {
            EffectTrack t;
            t.Property = TweenProperty::Alpha;
            t.HasFrom = true;
            t.From[0] = v;
            t.KeepTo = true;
            return t;
        }

        EffectTrack FromScale(float x, float y)
        {
            EffectTrack t;
            t.Property = TweenProperty::Scale;
            t.Channels = 2;
            t.HasFrom = true;
            t.From[0] = x;
            t.From[1] = y;
            t.KeepTo = true;
            return t;
        }

        EffectTrack FromScaleX(float x)
        {
            EffectTrack t;
            t.Property = TweenProperty::ScaleX;
            t.HasFrom = true;
            t.From[0] = x;
            t.KeepTo = true;
            return t;
        }

        EffectTrack FromPos(float x, float y)
        {
            EffectTrack t;
            t.Property = TweenProperty::Position;
            t.Channels = 2;
            t.HasFrom = true;
            t.RelativeFrom = true;
            t.From[0] = x;
            t.From[1] = y;
            t.KeepTo = true;
            t.Distance = true;
            return t;
        }

        EffectTrack FromRot(float degrees)
        {
            EffectTrack t;
            t.Property = TweenProperty::Rotation;
            t.HasFrom = true;
            t.RelativeFrom = true;
            t.From[0] = degrees;
            t.KeepTo = true;
            return t;
        }

        EffectTrack ToAlpha(float v)
        {
            EffectTrack t;
            t.Property = TweenProperty::Alpha;
            t.To[0] = v;
            return t;
        }

        EffectTrack ToScale(float x, float y)
        {
            EffectTrack t;
            t.Property = TweenProperty::Scale;
            t.Channels = 2;
            t.To[0] = x;
            t.To[1] = y;
            return t;
        }

        EffectTrack OffPos(float x, float y)
        {
            EffectTrack t;
            t.Property = TweenProperty::Position;
            t.Channels = 2;
            t.Relative = true;
            t.To[0] = x;
            t.To[1] = y;
            t.Distance = true;
            return t;
        }

        EffectTrack OffX(float x)
        {
            EffectTrack t;
            t.Property = TweenProperty::X;
            t.Relative = true;
            t.To[0] = x;
            t.Distance = true;
            return t;
        }

        EffectTrack OffY(float y)
        {
            EffectTrack t;
            t.Property = TweenProperty::Y;
            t.Relative = true;
            t.To[0] = y;
            t.Distance = true;
            return t;
        }

        EffectTrack OffScale(float x, float y)
        {
            EffectTrack t;
            t.Property = TweenProperty::Scale;
            t.Channels = 2;
            t.Relative = true;
            t.To[0] = x;
            t.To[1] = y;
            return t;
        }

        EffectTrack OffRot(float degrees)
        {
            EffectTrack t;
            t.Property = TweenProperty::Rotation;
            t.Relative = true;
            t.To[0] = degrees;
            return t;
        }

        EffectTrack OffAlpha(float delta)
        {
            EffectTrack t;
            t.Property = TweenProperty::Alpha;
            t.Relative = true;
            t.To[0] = delta * 255.0f;
            return t;
        }

        EffectSegment Seg(float length, const char *curve, std::vector<EffectTrack> tracks)
        {
            EffectSegment s;
            s.Length = length;
            s.Curve = curve;
            s.Tracks = std::move(tracks);
            return s;
        }

        EffectSegment Rest(float length)
        {
            EffectSegment s;
            s.Length = length;
            return s;
        }

        EffectLane Lane(std::vector<EffectSegment> segments, float delay = 0.0f)
        {
            EffectLane l;
            l.Delay = delay;
            l.Segments = std::move(segments);
            return l;
        }

        EffectLane One(const char *curve, std::vector<EffectTrack> tracks)
        {
            return Lane({Seg(1.0f, curve, std::move(tracks))});
        }

        std::vector<EffectPreset> BuildPresets()
        {
            std::vector<EffectPreset> out;
            auto add = [&](const char *name, const char *label, const char *category, const char *description, float duration, std::vector<EffectLane> lanes, bool loop = false)
            {
                EffectPreset p;
                p.Name = name;
                p.Label = label;
                p.Category = category;
                p.Description = description;
                p.Duration = duration;
                p.Loop = loop;
                p.Lanes = std::move(lanes);
                out.push_back(std::move(p));
            };
            const char *In = "Entrance";
            const char *Out = "Exit";
            const char *Attention = "Attention";
            const char *Loop = "Loop";

            add("fadeIn", "Fade In", In, "Fades from transparent to the current opacity.", 0.5f, {One("easeOut", {FromAlpha(0.0f)})});
            add("slideInLeft", "Slide In Left", In, "Slides in from the left while fading in.", 0.6f, {One("easeOutCubic", {FromPos(-200.0f, 0.0f)}), Lane({Seg(0.6f, "easeOut", {FromAlpha(0.0f)})})});
            add("slideInRight", "Slide In Right", In, "Slides in from the right while fading in.", 0.6f, {One("easeOutCubic", {FromPos(200.0f, 0.0f)}), Lane({Seg(0.6f, "easeOut", {FromAlpha(0.0f)})})});
            add("slideInTop", "Slide In Top", In, "Slides down into place from above while fading in.", 0.6f, {One("easeOutCubic", {FromPos(0.0f, 200.0f)}), Lane({Seg(0.6f, "easeOut", {FromAlpha(0.0f)})})});
            add("slideInBottom", "Slide In Bottom", In, "Slides up into place from below while fading in.", 0.6f, {One("easeOutCubic", {FromPos(0.0f, -200.0f)}), Lane({Seg(0.6f, "easeOut", {FromAlpha(0.0f)})})});
            add("popIn", "Pop In", In, "Grows from nothing with a small overshoot.", 0.5f, {One("easeOutBack", {FromScale(0.0f, 0.0f)}), Lane({Seg(0.4f, "easeOut", {FromAlpha(0.0f)})})});
            add("zoomIn", "Zoom In", In, "Scales up from a smaller size while fading in.", 0.5f, {One("easeOutCubic", {FromScale(0.3f, 0.3f)}), Lane({Seg(0.7f, "easeOut", {FromAlpha(0.0f)})})});
            add("dropIn", "Drop In", In, "Falls from above and bounces to rest.", 0.9f, {One("bounceOut", {FromPos(0.0f, 260.0f)}), Lane({Seg(0.25f, "easeOut", {FromAlpha(0.0f)})})});
            add("spinIn", "Spin In", In, "Spins and grows into place.", 0.7f, {One("easeOutCubic", {FromRot(-180.0f), FromScale(0.0f, 0.0f)}), Lane({Seg(0.5f, "easeOut", {FromAlpha(0.0f)})})});
            add("flipIn", "Flip In", In, "Opens horizontally like a card turning toward you.", 0.55f, {One("easeOutBack", {FromScaleX(0.0f)}), Lane({Seg(0.4f, "easeOut", {FromAlpha(0.0f)})})});
            add("riseIn", "Rise In", In, "A gentle upward drift while fading in.", 0.7f, {One("easeOut", {FromPos(0.0f, -40.0f)}), Lane({Seg(0.7f, "easeOut", {FromAlpha(0.0f)})})});
            add("elasticIn", "Elastic In", In, "Springs out of nothing and wobbles to rest.", 1.0f, {One("elasticOut", {FromScale(0.0f, 0.0f)}), Lane({Seg(0.2f, "easeOut", {FromAlpha(0.0f)})})});

            add("fadeOut", "Fade Out", Out, "Fades to transparent.", 0.5f, {One("easeIn", {ToAlpha(0.0f)})});
            add("slideOutLeft", "Slide Out Left", Out, "Slides away to the left while fading out.", 0.6f, {One("easeInCubic", {OffPos(-200.0f, 0.0f)}), Lane({Seg(0.6f, "easeIn", {ToAlpha(0.0f)})})});
            add("slideOutRight", "Slide Out Right", Out, "Slides away to the right while fading out.", 0.6f, {One("easeInCubic", {OffPos(200.0f, 0.0f)}), Lane({Seg(0.6f, "easeIn", {ToAlpha(0.0f)})})});
            add("slideOutTop", "Slide Out Top", Out, "Slides up and away while fading out.", 0.6f, {One("easeInCubic", {OffPos(0.0f, 200.0f)}), Lane({Seg(0.6f, "easeIn", {ToAlpha(0.0f)})})});
            add("slideOutBottom", "Slide Out Bottom", Out, "Slides down and away while fading out.", 0.6f, {One("easeInCubic", {OffPos(0.0f, -200.0f)}), Lane({Seg(0.6f, "easeIn", {ToAlpha(0.0f)})})});
            add("popOut", "Pop Out", Out, "Shrinks to nothing with a small wind-up.", 0.5f, {One("easeInBack", {ToScale(0.0f, 0.0f)}), Lane({Rest(0.6f), Seg(0.4f, "easeIn", {ToAlpha(0.0f)})})});
            add("zoomOut", "Zoom Out", Out, "Grows and fades away.", 0.5f, {One("easeOutCubic", {OffScale(0.6f, 0.6f)}), Lane({Seg(0.5f, "easeIn", {ToAlpha(0.0f)})})});
            add("dropOut", "Drop Out", Out, "Drops away with a small wind-up.", 0.6f, {One("easeInBack", {OffPos(0.0f, -260.0f)}), Lane({Rest(0.5f), Seg(0.5f, "easeIn", {ToAlpha(0.0f)})})});
            add("spinOut", "Spin Out", Out, "Spins and shrinks away.", 0.7f, {One("easeInCubic", {OffRot(180.0f), ToScale(0.0f, 0.0f)}), Lane({Rest(0.4f), Seg(0.6f, "easeIn", {ToAlpha(0.0f)})})});

            add("pulse", "Pulse", Attention, "Swells once and settles back.", 0.6f, {One("sine:0.5", {OffScale(0.15f, 0.15f)})});
            add("heartbeat", "Heartbeat", Attention, "Two quick beats, then a rest.", 0.9f, {Lane({Seg(0.2f, "sine:0.5", {OffScale(0.2f, 0.2f)}), Seg(0.2f, "sine:0.5", {OffScale(0.14f, 0.14f)}), Rest(0.6f)})});
            add("shake", "Shake", Attention, "A quick horizontal shake.", 0.5f, {One("sine:8", {OffX(14.0f)})});
            add("headShake", "Head Shake", Attention, "A short no-no shake with a little tilt.", 0.7f, {One("sine:3", {OffX(12.0f), OffRot(4.0f)})});
            add("wobble", "Wobble", Attention, "Rocks side to side and settles.", 0.8f, {One("sine:2", {OffX(18.0f), OffRot(10.0f)})});
            add("swing", "Swing", Attention, "Swings like a pendulum and settles.", 1.0f, {Lane({Seg(0.2f, "easeInOutSine", {OffRot(15.0f)}), Seg(0.2f, "easeInOutSine", {OffRot(-25.0f)}), Seg(0.2f, "easeInOutSine", {OffRot(18.0f)}), Seg(0.2f, "easeInOutSine", {OffRot(-10.0f)}), Seg(0.2f, "easeInOutSine", {OffRot(2.0f)})})});
            add("bounce", "Bounce", Attention, "Three hops of shrinking height.", 1.0f, {Lane({Seg(0.4f, "sine:0.5", {OffY(46.0f)}), Seg(0.3f, "sine:0.5", {OffY(22.0f)}), Seg(0.3f, "sine:0.5", {OffY(8.0f)})})});
            add("jump", "Jump", Attention, "One high jump.", 0.6f, {One("sine:0.5", {OffY(90.0f)})});
            add("rubberBand", "Rubber Band", Attention, "Stretches and snaps back like rubber.", 1.0f, {Lane({Seg(0.2f, "easeInOutSine", {OffScale(0.25f, -0.25f)}), Seg(0.2f, "easeInOutSine", {OffScale(-0.15f, 0.15f)}), Seg(0.2f, "easeInOutSine", {OffScale(0.1f, -0.1f)}), Seg(0.2f, "easeInOutSine", {OffScale(-0.05f, 0.05f)}), Seg(0.2f, "easeInOutSine", {OffScale(-0.15f, 0.15f)})})});
            add("tada", "Tada", Attention, "A burst of size and a celebratory wiggle.", 1.0f,
                {Lane({Seg(0.1f, "easeOut", {OffScale(-0.1f, -0.1f)}), Seg(0.1f, "easeOut", {OffScale(0.3f, 0.3f)}), Rest(0.6f), Seg(0.2f, "easeInOut", {OffScale(-0.2f, -0.2f)})}),
                 Lane({Seg(0.1f, "easeInOutSine", {OffRot(-3.0f)}), Seg(0.1f, "easeInOutSine", {OffRot(6.0f)}), Seg(0.1f, "easeInOutSine", {OffRot(-6.0f)}), Seg(0.1f, "easeInOutSine", {OffRot(6.0f)}), Seg(0.1f, "easeInOutSine", {OffRot(-6.0f)}), Seg(0.1f, "easeInOutSine", {OffRot(6.0f)}), Seg(0.1f, "easeInOutSine", {OffRot(-6.0f)}), Seg(0.1f, "easeInOutSine", {OffRot(3.0f)}), Rest(0.2f)})});
            add("flash", "Flash", Attention, "Blinks three times.", 0.9f, {Lane({Seg(0.333f, "sine:0.5", {OffAlpha(-1.0f)}), Seg(0.333f, "sine:0.5", {OffAlpha(-1.0f)}), Seg(0.334f, "sine:0.5", {OffAlpha(-1.0f)})})});
            add("spinOnce", "Spin Once", Attention, "One full turn.", 0.8f, {One("easeInOutCubic", {OffRot(360.0f)})});

            add("float", "Float", Loop, "Bobs gently up and down, forever.", 2.4f, {One("sine:1", {OffY(14.0f)})}, true);
            add("breathe", "Breathe", Loop, "Slowly grows and shrinks, forever.", 3.0f, {One("sine:1", {OffScale(0.06f, 0.06f)})}, true);
            add("spin", "Spin", Loop, "Turns at a steady pace, forever.", 3.0f, {One("linear", {OffRot(-360.0f)})}, true);
            add("sway", "Sway", Loop, "Rocks slowly left and right, forever.", 2.5f, {One("sine:1", {OffRot(6.0f)})}, true);
            add("blink", "Blink", Loop, "Dips in and out of view, forever.", 1.2f, {One("sine:0.5", {OffAlpha(-0.7f)})}, true);
            return out;
        }

        Curve MakeCurve(const std::string &name)
        {
            if (name.rfind("sine:", 0) == 0)
                return Curve::Sine(static_cast<float>(std::atof(name.c_str() + 5)));
            Curve curve;
            if (Curve::FromName(name, curve))
                return curve;
            return Curve::Linear();
        }

        TweenTrack MakeTrack(const EffectTrack &spec, const EffectOptions &options)
        {
            TweenTrack track;
            track.Property = spec.Property;
            track.Channels = spec.Channels;
            const float distance = spec.Distance ? options.Distance : 1.0f;
            const float amount = (spec.Relative && !spec.Distance) ? options.Amount : 1.0f;
            for (int c = 0; c < 4; ++c)
            {
                track.From[c] = Keep;
                track.To[c] = Keep;
            }
            for (int c = 0; c < spec.Channels && c < 2; ++c)
            {
                track.From[c] = spec.HasFrom ? spec.From[c] * distance : Keep;
                track.To[c] = spec.KeepTo ? Keep : spec.To[c] * distance * amount;
            }
            track.ExplicitFrom = spec.HasFrom;
            track.RelativeFrom = spec.HasFrom && spec.RelativeFrom;
            track.Relative = spec.Relative;
            return track;
        }

        std::string Num(float v)
        {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%.3f", v);
            std::string s = buffer;
            while (!s.empty() && s.back() == '0')
                s.pop_back();
            if (!s.empty() && s.back() == '.')
                s.pop_back();
            if (s == "-0" || s.empty())
                s = "0";
            return s;
        }

        std::string Signed(float v)
        {
            return std::string(v < 0.0f ? "\"-=" : "\"+=") + Num(std::fabs(v)) + "\"";
        }

        std::string CurveCode(const std::string &curve)
        {
            if (curve.rfind("sine:", 0) == 0)
                return "Curves.Sine(" + Num(static_cast<float>(std::atof(curve.c_str() + 5))) + ")";
            return "\"" + curve + "\"";
        }

        std::string PropsCode(const EffectSegment &segment, const EffectOptions &options)
        {
            std::string out = "{";
            bool first = true;
            auto put = [&](const std::string &key, const std::string &value)
            {
                if (!first)
                    out += ", ";
                first = false;
                out += key + " = " + value;
            };
            for (const EffectTrack &spec : segment.Tracks)
            {
                const TweenTrack track = MakeTrack(spec, options);
                const bool from = spec.HasFrom;
                const bool relative = from ? spec.RelativeFrom : spec.Relative;
                const float *values = from ? track.From : track.To;
                auto value = [&](int c)
                { return relative ? Signed(values[c]) : Num(values[c]); };
                switch (spec.Property)
                {
                case TweenProperty::Position:
                    put("x", value(0));
                    put("y", value(1));
                    break;
                case TweenProperty::Scale:
                    put("scaleX", value(0));
                    put("scaleY", value(1));
                    break;
                default:
                    put(TweenSystem::PropertyName(spec.Property), value(0));
                    break;
                }
            }
            return out + "}";
        }

        std::string Capitalised(const std::string &name)
        {
            std::string s = name;
            if (!s.empty())
                s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
            return s;
        }
    }

    namespace EffectPresets
    {
        const std::vector<EffectPreset> &All()
        {
            static const std::vector<EffectPreset> presets = BuildPresets();
            return presets;
        }

        const EffectPreset *Find(const std::string &name)
        {
            for (const EffectPreset &preset : All())
                if (preset.Name == name)
                    return &preset;
            auto lower = [](std::string s)
            {
                for (char &c : s)
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return s;
            };
            const std::string wanted = lower(name);
            for (const EffectPreset &preset : All())
                if (lower(preset.Name) == wanted)
                    return &preset;
            return nullptr;
        }

        std::vector<std::string> Categories()
        {
            std::vector<std::string> out;
            for (const EffectPreset &preset : All())
                if (std::find(out.begin(), out.end(), preset.Category) == out.end())
                    out.push_back(preset.Category);
            return out;
        }

        uint32_t Play(TweenSystem &system, entt::entity target, const EffectPreset &preset, const EffectOptions &options)
        {
            const float duration = options.Duration > 0.0f ? options.Duration : preset.Duration;
            std::vector<uint32_t> lanes;
            std::vector<uint32_t> immediate;
            for (const EffectLane &lane : preset.Lanes)
            {
                std::vector<uint32_t> children;
                if (lane.Delay > 0.0f)
                    children.push_back(system.Delay(lane.Delay * duration));
                for (const EffectSegment &segment : lane.Segments)
                {
                    const float seconds = segment.Length * duration;
                    if (segment.Tracks.empty())
                    {
                        children.push_back(system.Delay(seconds));
                        continue;
                    }
                    std::vector<TweenTrack> tracks;
                    for (const EffectTrack &spec : segment.Tracks)
                        tracks.push_back(MakeTrack(spec, options));
                    const uint32_t child = system.To(target, std::move(tracks), seconds, MakeCurve(segment.Curve));
                    if (children.empty() && segment.Tracks.front().HasFrom)
                        immediate.push_back(child);
                    children.push_back(child);
                }
                if (children.empty())
                    continue;
                if (children.size() == 1)
                    lanes.push_back(children.front());
                else if (const uint32_t group = system.Group(true, children))
                    lanes.push_back(group);
            }
            if (lanes.empty())
                return 0;
            uint32_t id = lanes.front();
            if (lanes.size() > 1)
            {
                id = system.Group(false, lanes);
                if (!id)
                    return 0;
            }
            if (TweenNode *node = system.Get(id))
            {
                node->Delay += std::max(options.Delay, 0.0f);
                node->Speed = options.Speed > 0.0f ? options.Speed : 1.0f;
                const int loop = options.Loop == -2 ? (preset.Loop ? -1 : 0) : options.Loop;
                node->Repeat = loop;
            }
            if (options.Delay > 0.0f)
                for (uint32_t child : immediate)
                    system.ImmediateRender(child);
            return id;
        }

        std::string OneLiner(const EffectPreset &preset)
        {
            return "Effect." + Capitalised(preset.Name) + "(self.entity)";
        }

        std::string PlayCall(const EffectPreset &preset, const EffectOptions &options)
        {
            std::string opts;
            auto add = [&](const std::string &text)
            {
                opts += (opts.empty() ? "" : ", ") + text;
            };
            if (options.Duration > 0.0f)
                add("time = " + Num(options.Duration));
            if (options.Delay > 0.0f)
                add("delay = " + Num(options.Delay));
            if (options.Distance != 1.0f)
                add("distance = " + Num(options.Distance));
            if (options.Amount != 1.0f)
                add("amount = " + Num(options.Amount));
            if (options.Speed != 1.0f)
                add("speed = " + Num(options.Speed));
            if (options.Loop != -2)
                add(options.Loop < 0 ? "loop = true" : "loop = " + std::to_string(options.Loop));
            return "Effect.Play(self.entity, \"" + preset.Name + "\"" + (opts.empty() ? "" : ", {" + opts + "}") + ")";
        }

        std::string ExpandedLua(const EffectPreset &preset, const EffectOptions &options)
        {
            const float duration = options.Duration > 0.0f ? options.Duration : preset.Duration;
            auto segmentCode = [&](const EffectSegment &segment) -> std::string
            {
                const std::string seconds = Num(segment.Length * duration);
                if (segment.Tracks.empty())
                    return seconds;
                const char *fn = segment.Tracks.front().HasFrom ? "Tween.From" : "Tween.To";
                return std::string(fn) + "(e, " + PropsCode(segment, options) + ", " + seconds + ", " + CurveCode(segment.Curve) + ")";
            };

            std::vector<std::string> laneCodes;
            for (const EffectLane &lane : preset.Lanes)
            {
                std::vector<std::string> items;
                if (lane.Delay > 0.0f)
                    items.push_back(Num(lane.Delay * duration));
                for (const EffectSegment &segment : lane.Segments)
                    items.push_back(segmentCode(segment));
                if (items.size() == 1)
                    laneCodes.push_back(items.front());
                else
                {
                    std::string code = "Tween.Sequence({\n";
                    for (size_t i = 0; i < items.size(); ++i)
                        code += "        " + items[i] + (i + 1 < items.size() ? "," : "") + "\n";
                    laneCodes.push_back(code + "    })");
                }
            }

            const int loop = options.Loop == -2 ? (preset.Loop ? -1 : 0) : options.Loop;
            std::string tail;
            if (loop != 0)
                tail = loop < 0 ? "loop = true" : "loop = " + std::to_string(loop);
            if (options.Delay > 0.0f)
                tail += (tail.empty() ? "" : ", ") + std::string("delay = ") + Num(options.Delay);
            if (options.Speed != 1.0f)
                tail += (tail.empty() ? "" : ", ") + std::string("speed = ") + Num(options.Speed);

            std::string code = "local e = self.entity\n";
            if (laneCodes.size() == 1)
            {
                std::string only = laneCodes.front();
                if (!tail.empty() && !only.empty() && only.back() == ')')
                {
                    only.pop_back();
                    only += ", {" + tail + "})";
                }
                return code + only + "\n";
            }
            std::string joined;
            for (size_t i = 0; i < laneCodes.size(); ++i)
                joined += "    " + laneCodes[i] + (i + 1 < laneCodes.size() ? "," : "") + "\n";
            return code + "Tween.Parallel({\n" + joined + "}" + (tail.empty() ? "" : ", {" + tail + "}") + ")\n";
        }
    }
}
