#include <Engine/Animation/MotionPresets.h>

#include <Engine/Animation/Curves.h>
#include <Engine/Animation/Tween.h>
#include <Engine/Rendering/Font.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Vector/VectorPath.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <utility>

namespace SIMPEngine
{
    namespace
    {
        constexpr float Pi = 3.14159265358979f;

        const glm::vec4 Teal{0.36f, 0.83f, 0.84f, 1.0f};
        const glm::vec4 Gold{1.0f, 0.78f, 0.35f, 1.0f};
        const glm::vec4 Rose{1.0f, 0.42f, 0.52f, 1.0f};
        const glm::vec4 Violet{0.66f, 0.52f, 1.0f, 1.0f};
        const glm::vec4 Lime{0.52f, 0.86f, 0.45f, 1.0f};
        const glm::vec4 Sky{0.40f, 0.62f, 1.0f, 1.0f};
        const glm::vec4 Orange{1.0f, 0.58f, 0.30f, 1.0f};
        const glm::vec4 White{0.94f, 0.95f, 0.97f, 1.0f};

        glm::vec4 PaletteColor(size_t i, const glm::vec4 &first, const glm::vec4 &second)
        {
            static const glm::vec4 rest[] = {Rose, Violet, Lime, Sky, Orange};
            if (i == 0)
                return first;
            if (i == 1)
                return second;
            return rest[(i - 2) % 5];
        }

        SDL_Color ToSDL(const glm::vec4 &c)
        {
            auto b = [](float v)
            { return static_cast<Uint8>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
            return {b(c.r), b(c.g), b(c.b), b(c.a)};
        }

        glm::vec4 WithA(glm::vec4 c, float a)
        {
            c.a = a;
            return c;
        }

        glm::vec2 Dir(float degrees)
        {
            const float r = glm::radians(degrees);
            return {std::cos(r), std::sin(r)};
        }

        std::vector<glm::vec2> Arc(glm::vec2 c, float radius, float fromDeg, float toDeg, int segments = 48)
        {
            std::vector<glm::vec2> pts;
            segments = std::max(segments, 2);
            for (int i = 0; i <= segments; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(segments);
                pts.push_back(c + Dir(fromDeg + (toDeg - fromDeg) * t) * radius);
            }
            return pts;
        }

        float Length(const std::vector<glm::vec2> &pts)
        {
            float l = 0.0f;
            for (size_t i = 1; i < pts.size(); ++i)
                l += glm::length(pts[i] - pts[i - 1]);
            return l;
        }

        float Angle(glm::vec2 v)
        {
            return glm::degrees(std::atan2(v.y, v.x));
        }

        VectorPath ArrowHead(float s)
        {
            return VectorPath::Points({{-s * 0.9f, s * 0.62f}, {s * 0.55f, 0.0f}, {-s * 0.9f, -s * 0.62f}}, true);
        }

        VectorPath Pie(float radius, float fromDeg, float toDeg)
        {
            std::vector<glm::vec2> pts{{0.0f, 0.0f}};
            const int segments = std::max(4, static_cast<int>(std::abs(toDeg - fromDeg) / 4.0f));
            for (glm::vec2 p : Arc({0.0f, 0.0f}, radius, fromDeg, toDeg, segments))
                pts.push_back(p);
            return VectorPath::Points(pts, true);
        }

        VectorPath Bar(float width, float height, float radius)
        {
            const float r = std::min(radius, std::min(width * 0.5f, std::abs(height)));
            std::vector<glm::vec2> pts{{-width * 0.5f, 0.0f}, {width * 0.5f, 0.0f}};
            if (r > 0.5f && height > 0.0f)
            {
                for (glm::vec2 p : Arc({width * 0.5f - r, height - r}, r, 0.0f, 90.0f, 6))
                    pts.push_back(p);
                for (glm::vec2 p : Arc({-width * 0.5f + r, height - r}, r, 90.0f, 180.0f, 6))
                    pts.push_back(p);
            }
            else
            {
                pts.push_back({width * 0.5f, height});
                pts.push_back({-width * 0.5f, height});
            }
            return VectorPath::Points(pts, true);
        }

        struct Tr
        {
            TweenProperty Property;
            int Channels;
            float From[4];
            float To[4];
        };

        Tr One(TweenProperty p, float from, float to)
        {
            return Tr{p, 1, {from, 0.0f, 0.0f, 0.0f}, {to, 0.0f, 0.0f, 0.0f}};
        }

        Tr Two(TweenProperty p, glm::vec2 from, glm::vec2 to)
        {
            return Tr{p, 2, {from.x, from.y, 0.0f, 0.0f}, {to.x, to.y, 0.0f, 0.0f}};
        }

        Tr Four(TweenProperty p, glm::vec4 from, glm::vec4 to)
        {
            return Tr{p, 4, {from.x, from.y, from.z, from.w}, {to.x, to.y, to.z, to.w}};
        }

        Tr ScaleT(float from, float to)
        {
            return Two(TweenProperty::Scale, {from, from}, {to, to});
        }

        Tr MoveT(glm::vec2 from, glm::vec2 to)
        {
            return Two(TweenProperty::Position, from, to);
        }

        Tr AlphaT(float from, float to)
        {
            return One(TweenProperty::Alpha, from * 255.0f, to * 255.0f);
        }
    }

    class MotionBuilder
    {
    public:
        MotionBuilder(Scene &scene, const MotionPreset &preset, const MotionOptions &options)
            : S(scene), T(scene.GetTweens()), R(scene.GetRegistry()), O(options)
        {
            TimeScale = options.Duration > 0.0f && preset.Duration > 0.0f ? options.Duration / preset.Duration : 1.0f;
            Size = options.Size > 0.0f ? options.Size : 0.0f;
            Color = options.Color.x >= 0.0f ? options.Color : Teal;
            Accent = options.Accent.x >= 0.0f ? options.Accent : Gold;
            Entity root = scene.CreateEntity(preset.Name);
            Root = root.GetHandle();
            auto &tf = R.get<TransformComponent>(Root);
            tf.position = options.Position;
            tf.zIndex = options.Z;
            if (options.Parent != entt::null && R.valid(options.Parent))
                scene.SetParent(Root, options.Parent);
        }

        Scene &S;
        TweenSystem &T;
        entt::registry &R;
        const MotionOptions &O;
        entt::entity Root = entt::null;
        float TimeScale = 1.0f;
        float Size = 0.0f;
        glm::vec4 Color;
        glm::vec4 Accent;
        float End = 0.0f;

        float SizeOr(float fallback) const { return Size > 0.0f ? Size : fallback; }
        float WidthOr(float fallback) const { return O.Width > 0.0f ? O.Width : fallback; }
        glm::vec4 ColorOr(const glm::vec4 &fallback) const { return O.Color.x >= 0.0f ? O.Color : fallback; }
        std::string TextOr(const char *fallback) const { return O.Text.empty() ? std::string(fallback) : O.Text; }
        std::vector<glm::vec2> PointsOr(std::vector<glm::vec2> fallback) const { return O.Points.size() >= 2 ? O.Points : fallback; }
        std::vector<float> ValuesOr(std::vector<float> fallback) const { return O.Values.empty() ? fallback : O.Values; }

        entt::entity Node(const char *name, glm::vec2 pos, float z = 0.0f, entt::entity parent = entt::null)
        {
            Entity e = S.CreateEntity(name);
            auto &tf = e.GetComponent<TransformComponent>();
            tf.position = pos;
            tf.zIndex = z;
            S.SetParent(e.GetHandle(), parent == entt::null ? Root : parent);
            return e.GetHandle();
        }

        entt::entity Fill(const VectorPath &path, glm::vec2 pos, const glm::vec4 &color, float z = 0.0f, entt::entity parent = entt::null)
        {
            const entt::entity e = Node("Shape", pos, z, parent);
            auto &v = R.emplace<VectorShapeComponent>(e);
            v.path = path;
            v.fill = true;
            v.fillColor = ToSDL(color);
            v.stroke = false;
            return e;
        }

        entt::entity Stroke(const VectorPath &path, glm::vec2 pos, const glm::vec4 &color, float width, float z = 0.0f, entt::entity parent = entt::null)
        {
            const entt::entity e = Node("Stroke", pos, z, parent);
            auto &v = R.emplace<VectorShapeComponent>(e);
            v.path = path;
            v.fill = false;
            v.stroke = true;
            v.strokeColor = ToSDL(color);
            v.strokeWidth = width;
            return e;
        }

        entt::entity Line(const std::vector<glm::vec2> &pts, const glm::vec4 &color, float width, float z = 0.0f, bool closed = false, entt::entity parent = entt::null)
        {
            return Stroke(VectorPath::Points(pts, closed), {0.0f, 0.0f}, color, width, z, parent);
        }

        entt::entity Label(const std::string &text, glm::vec2 pos, float size, const glm::vec4 &color, float z = 0.0f, int align = 1, entt::entity parent = entt::null)
        {
            const entt::entity e = Node("Text", pos, z, parent);
            auto &t = R.emplace<TextComponent>(e);
            t.Text = text;
            t.Size = size;
            t.Color = ToSDL(color);
            t.Align = align;
            return e;
        }

        void Glow(entt::entity e, const glm::vec4 &color, float intensity = 1.6f, float spread = 1.0f)
        {
            auto &g = R.emplace_or_replace<GlowComponent>(e);
            g.color = ToSDL(color);
            g.intensity = intensity;
            g.spread = spread;
        }

        TransformComponent &Tf(entt::entity e) { return R.get<TransformComponent>(e); }
        VectorShapeComponent &V(entt::entity e) { return R.get<VectorShapeComponent>(e); }

        void SetScale(entt::entity e, float s) { Tf(e).scale = {s, s}; }
        void SetAlpha(entt::entity e, float a)
        {
            const Uint8 v = static_cast<Uint8>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
            if (auto *shape = R.try_get<VectorShapeComponent>(e))
            {
                shape->fillColor.a = v;
                shape->strokeColor.a = v;
            }
            if (auto *text = R.try_get<TextComponent>(e))
                text->Color.a = v;
        }
        void SetTrim(entt::entity e, float start, float end)
        {
            V(e).trimStart = start;
            V(e).trimEnd = end;
        }

        uint32_t Delayed(uint32_t id, float t, float d, int repeat = 0, bool yoyo = false)
        {
            if (TweenNode *node = T.Get(id))
            {
                node->Delay = O.Delay + t * TimeScale;
                node->Repeat = repeat;
                node->Yoyo = yoyo;
            }
            End = std::max(End, t + d * static_cast<float>(repeat + 1));
            return id;
        }

        static Curve CurveOf(const char *name)
        {
            Curve c;
            if (!Curve::FromName(name, c))
                Curve::FromName("easeOut", c);
            return c;
        }

        uint32_t To(entt::entity e, std::initializer_list<Tr> tracks, float t, float d, const char *curve = "easeOutCubic", int repeat = 0, bool yoyo = false)
        {
            return Tracks(e, tracks.begin(), tracks.size(), t, d, curve, repeat, yoyo);
        }

        uint32_t Tracks(entt::entity e, const Tr *tracks, size_t count, float t, float d, const char *curve, int repeat, bool yoyo);

        uint32_t Along(entt::entity e, std::vector<glm::vec2> pts, float t, float d, const char *curve = "easeInOutCubic", bool orient = false)
        {
            return Delayed(T.Motion(e, std::move(pts), false, d * TimeScale, CurveOf(curve), orient, false), t, d);
        }

        uint32_t Value(float t, float d, const char *curve, std::function<void(float)> fn, int repeat = 0, bool yoyo = false)
        {
            return Delayed(T.Value(0.0f, 1.0f, d * TimeScale, CurveOf(curve), std::move(fn)), t, d, repeat, yoyo);
        }

        void Pop(entt::entity e, float t, float d = 0.45f, const char *curve = "easeOutBack")
        {
            SetScale(e, 0.0f);
            To(e, {ScaleT(0.0f, 1.0f)}, t, d, curve);
        }

        void FadeIn(entt::entity e, float t, float d = 0.35f)
        {
            SetAlpha(e, 0.0f);
            To(e, {AlphaT(0.0f, 1.0f)}, t, d, "easeOut");
        }

        void Draw(entt::entity e, float t, float d, const char *curve = "easeInOutCubic")
        {
            SetTrim(e, 0.0f, 0.0f);
            To(e, {One(TweenProperty::TrimEnd, 0.0f, 1.0f)}, t, d, curve);
        }

        void Fly(entt::entity e, glm::vec2 from, glm::vec2 to, float t, float d, const char *curve = "easeOutCubic")
        {
            Tf(e).position = from;
            To(e, {MoveT(from, to)}, t, d, curve);
        }

        void SetText(entt::entity e, const std::string &text)
        {
            if (R.valid(e))
                if (auto *component = R.try_get<TextComponent>(e))
                    component->Text = text;
        }
    };

    uint32_t MotionBuilder::Tracks(entt::entity e, const Tr *tracks, size_t count, float t, float d, const char *curve, int repeat, bool yoyo)
    {
        std::vector<TweenTrack> list(count);
        for (size_t i = 0; i < count; ++i)
        {
            TweenTrack &track = list[i];
            track.Property = tracks[i].Property;
            track.Channels = tracks[i].Channels;
            std::copy(tracks[i].From, tracks[i].From + 4, track.From);
            std::copy(tracks[i].To, tracks[i].To + 4, track.To);
            track.ExplicitFrom = true;
        }
        return Delayed(T.To(e, std::move(list), d * TimeScale, CurveOf(curve)), t, d, repeat, yoyo);
    }

    namespace
    {
        using Builder = std::function<void(MotionBuilder &)>;

        struct ShapeKind
        {
            const char *Name;
            std::function<VectorPath(float)> Path;
        };

        const std::vector<ShapeKind> &ShapeKinds()
        {
            static const std::vector<ShapeKind> kinds = {
                {"Circle", [](float r) { return VectorPath::Circle(r); }},
                {"Square", [](float r) { return VectorPath::Rect(r * 1.8f, r * 1.8f, r * 0.22f); }},
                {"Triangle", [](float r) { return VectorPath::Polygon(3, r * 1.15f); }},
                {"Star", [](float r) { return VectorPath::Star(5, r * 1.15f, r * 0.5f); }},
                {"Heart", [](float r) { return VectorPath::Heart(r * 2.0f); }},
                {"Hexagon", [](float r) { return VectorPath::Polygon(6, r * 1.05f); }},
            };
            return kinds;
        }

        void ShapeEntrance(MotionBuilder &b, const ShapeKind &kind, const std::string &style)
        {
            const float r = b.SizeOr(60.0f);
            const VectorPath path = kind.Path(r);
            if (style == "Pop")
            {
                b.Pop(b.Fill(path, {0, 0}, b.Color), 0.0f, 0.5f);
            }
            else if (style == "GlowPop")
            {
                const entt::entity e = b.Fill(path, {0, 0}, b.Color);
                b.Glow(e, b.Color, 0.0f, 1.2f);
                b.Pop(e, 0.0f, 0.5f);
                b.To(e, {One(TweenProperty::GlowIntensity, 0.0f, 3.0f)}, 0.0f, 0.35f, "easeOut");
                b.To(e, {One(TweenProperty::GlowIntensity, 3.0f, 1.3f)}, 0.35f, 0.65f, "easeInOut");
            }
            else if (style == "Grow")
            {
                const entt::entity e = b.Fill(path, {0, 0}, b.Color);
                b.SetScale(e, 0.0f);
                b.FadeIn(e, 0.0f, 0.4f);
                b.To(e, {ScaleT(0.0f, 1.0f)}, 0.0f, 0.7f, "easeOutCubic");
            }
            else if (style == "Fade")
            {
                const entt::entity e = b.Fill(path, {0, 0}, b.Color);
                b.FadeIn(e, 0.0f, 0.6f);
                b.Fly(e, {0.0f, -r * 0.35f}, {0.0f, 0.0f}, 0.0f, 0.6f);
            }
            else if (style == "Spin")
            {
                const entt::entity e = b.Fill(path, {0, 0}, b.Color);
                b.SetScale(e, 0.0f);
                b.Tf(e).rotation = -200.0f;
                b.To(e, {ScaleT(0.0f, 1.0f), One(TweenProperty::Rotation, -200.0f, 0.0f)}, 0.0f, 0.8f, "easeOutCubic");
            }
            else if (style == "Drop")
            {
                const entt::entity e = b.Fill(path, {0, 0}, b.Color);
                b.FadeIn(e, 0.0f, 0.2f);
                b.Fly(e, {0.0f, r * 4.0f}, {0.0f, 0.0f}, 0.0f, 0.9f, "bounceOut");
            }
            else if (style == "Elastic")
            {
                b.Pop(b.Fill(path, {0, 0}, b.Color), 0.0f, 1.1f, "elasticOut");
            }
            else if (style == "Draw")
            {
                const entt::entity fill = b.Fill(path, {0, 0}, b.Color, 0.0f);
                b.FadeIn(fill, 0.75f, 0.4f);
                const entt::entity outline = b.Stroke(path, {0, 0}, b.Color, b.WidthOr(5.0f), 0.01f);
                b.Draw(outline, 0.0f, 0.8f);
            }
            else if (style == "Ripple")
            {
                b.Pop(b.Fill(path, {0, 0}, b.Color, 0.01f), 0.0f, 0.45f);
                for (int i = 0; i < 2; ++i)
                {
                    const entt::entity ring = b.Stroke(path, {0, 0}, b.Color, b.WidthOr(4.0f), 0.0f);
                    b.SetAlpha(ring, 0.0f);
                    const float t = 0.25f + 0.25f * static_cast<float>(i);
                    b.To(ring, {ScaleT(1.0f, 2.2f), AlphaT(0.9f, 0.0f)}, t, 0.8f, "easeOutCubic");
                }
            }
        }

        std::vector<glm::vec2> DefaultPath()
        {
            return {{-170.0f, -60.0f}, {-60.0f, 70.0f}, {60.0f, -40.0f}, {170.0f, 60.0f}};
        }

        std::vector<glm::vec2> Local(const MotionBuilder &b, std::vector<glm::vec2> fallback)
        {
            return b.PointsOr(std::move(fallback));
        }

        void BuildArrow(MotionBuilder &b, const std::vector<glm::vec2> &pts, bool slide, bool twoHeads, bool glow)
        {
            const float width = b.WidthOr(7.0f);
            const float draw = 0.8f;
            const glm::vec2 last = pts.back();
            const glm::vec2 lastDir = glm::normalize(pts.back() - pts[pts.size() - 2]);
            std::vector<glm::vec2> path = pts;
            float f0 = 1.0f;
            const float slideLength = b.SizeOr(320.0f);
            if (slide)
            {
                path.push_back(last + lastDir * slideLength);
                f0 = Length(pts) / Length(path);
            }
            const entt::entity body = b.Line(path, b.Color, width, 0.0f);
            if (glow)
                b.Glow(body, b.Color, 1.4f, 1.0f);
            b.SetTrim(body, 0.0f, 0.0f);
            b.To(body, {One(TweenProperty::TrimEnd, 0.0f, f0)}, 0.0f, draw, "easeInOutCubic");

            const entt::entity head = b.Fill(ArrowHead(width * 2.6f), pts.front(), b.Color, 0.02f);
            b.Tf(head).rotation = Angle(pts[1] - pts[0]);
            b.Along(head, pts, 0.0f, draw, "easeInOutCubic", true);
            if (glow)
                b.Glow(head, b.Color, 1.4f, 1.0f);

            if (twoHeads)
            {
                const entt::entity tail = b.Fill(ArrowHead(width * 2.6f), pts.front(), b.Color, 0.02f);
                b.Tf(tail).rotation = Angle(pts[0] - pts[1]);
                b.Pop(tail, 0.0f, 0.35f);
            }

            if (slide)
            {
                const float t = draw + 0.35f;
                b.To(body, {One(TweenProperty::TrimEnd, f0, 1.0f), One(TweenProperty::TrimStart, 0.0f, 1.0f - f0)}, t, 0.9f, "easeInCubic");
                b.Along(head, {last, last + lastDir * slideLength}, t, 0.9f, "easeInCubic", false);
                b.To(body, {AlphaT(1.0f, 0.0f)}, t + 0.55f, 0.35f, "easeIn");
                b.To(head, {AlphaT(1.0f, 0.0f)}, t + 0.55f, 0.35f, "easeIn");
            }
        }

        struct Chart
        {
            float W, H;
            glm::vec2 Map(float x, float y, float x0, float x1, float y0, float y1) const
            {
                const float u = x1 > x0 ? (x - x0) / (x1 - x0) : 0.5f;
                const float v = y1 > y0 ? (y - y0) / (y1 - y0) : 0.5f;
                return {-W * 0.5f + u * W, -H * 0.5f + v * H};
            }
        };

        void Axes(MotionBuilder &b, const Chart &c, float originY, float t, bool ticks)
        {
            const glm::vec4 axis = WithA(White, 0.75f);
            const entt::entity x = b.Line({{-c.W * 0.5f - 10.0f, originY}, {c.W * 0.5f + 16.0f, originY}}, axis, 2.5f, -0.05f);
            const entt::entity y = b.Line({{-c.W * 0.5f, -c.H * 0.5f - 10.0f}, {-c.W * 0.5f, c.H * 0.5f + 16.0f}}, axis, 2.5f, -0.05f);
            b.Draw(x, t, 0.5f);
            b.Draw(y, t + 0.1f, 0.5f);
            const entt::entity hx = b.Fill(ArrowHead(8.0f), {c.W * 0.5f + 18.0f, originY}, axis, -0.04f);
            const entt::entity hy = b.Fill(ArrowHead(8.0f), {-c.W * 0.5f, c.H * 0.5f + 18.0f}, axis, -0.04f);
            b.Tf(hy).rotation = 90.0f;
            b.Pop(hx, t + 0.45f, 0.3f);
            b.Pop(hy, t + 0.55f, 0.3f);
            if (!ticks)
                return;
            for (int i = 1; i <= 5; ++i)
            {
                const float px = -c.W * 0.5f + c.W * static_cast<float>(i) / 5.0f;
                const entt::entity tick = b.Line({{px, originY - 5.0f}, {px, originY + 5.0f}}, axis, 2.0f, -0.05f);
                b.FadeIn(tick, t + 0.3f + 0.05f * static_cast<float>(i), 0.2f);
            }
        }

        struct Samples
        {
            std::vector<glm::vec2> Points;
            float OriginY = 0.0f;
        };

        Samples FitSamples(const MotionBuilder &b, const Chart &c, const std::function<float(float)> &fallback, float x0, float x1)
        {
            Samples s;
            if (b.O.Points.size() >= 2)
            {
                s.Points = b.O.Points;
                s.OriginY = 0.0f;
                return s;
            }
            std::vector<float> ys = b.O.Values.size() >= 2 ? b.O.Values : MotionPresets::SampleFunction(fallback, x0, x1, 96);
            float lo = *std::min_element(ys.begin(), ys.end());
            float hi = *std::max_element(ys.begin(), ys.end());
            if (hi - lo < 1e-5f)
            {
                lo -= 1.0f;
                hi += 1.0f;
            }
            const float pad = (hi - lo) * 0.1f;
            lo -= pad;
            hi += pad;
            for (size_t i = 0; i < ys.size(); ++i)
            {
                const float x = static_cast<float>(i) / static_cast<float>(ys.size() - 1);
                s.Points.push_back(c.Map(x, ys[i], 0.0f, 1.0f, lo, hi));
            }
            s.OriginY = (lo <= 0.0f && hi >= 0.0f) ? c.Map(0.0f, 0.0f, 0.0f, 1.0f, lo, hi).y : -c.H * 0.5f;
            return s;
        }

        void Add(std::vector<MotionPreset> &list, std::string name, std::string category, std::string description, float duration, Builder build, MotionInput input = MotionInput::None)
        {
            MotionPreset p;
            p.Name = std::move(name);
            p.Category = std::move(category);
            p.Description = std::move(description);
            p.Duration = duration;
            p.Build = std::move(build);
            p.Input = input;
            list.push_back(std::move(p));
        }

        float Hash(int i)
        {
            const float v = std::sin(static_cast<float>(i) * 12.9898f + 78.233f) * 43758.5453f;
            return v - std::floor(v);
        }

        void ShapeExit(MotionBuilder &b, const ShapeKind &kind, const std::string &style)
        {
            const float r = b.SizeOr(60.0f);
            const VectorPath path = kind.Path(r);
            const entt::entity e = b.Fill(path, {0, 0}, b.Color, 0.01f);
            const float t = 0.35f;
            if (style == "PopOut")
            {
                b.To(e, {ScaleT(1.0f, 1.15f)}, t, 0.12f, "easeOut");
                b.To(e, {ScaleT(1.15f, 0.0f)}, t + 0.12f, 0.35f, "easeInBack");
            }
            else if (style == "FadeOut")
            {
                b.To(e, {AlphaT(1.0f, 0.0f)}, t, 0.5f, "easeIn");
                b.To(e, {MoveT({0, 0}, {0.0f, r * 0.5f})}, t, 0.5f, "easeInCubic");
            }
            else if (style == "SpinOut")
                b.To(e, {ScaleT(1.0f, 0.0f), One(TweenProperty::Rotation, 0.0f, 240.0f)}, t, 0.6f, "easeInCubic");
            else if (style == "Explode")
            {
                b.To(e, {ScaleT(1.0f, 1.2f)}, t, 0.15f, "easeOut");
                b.To(e, {AlphaT(1.0f, 0.0f)}, t + 0.15f, 0.08f, "linear");
                for (int i = 0; i < 12; ++i)
                {
                    const glm::vec2 d = Dir(30.0f * static_cast<float>(i) + 15.0f * Hash(i));
                    const entt::entity shard = b.Fill(kind.Path(r * 0.22f), d * r * 0.3f, PaletteColor(i % 3, b.Color, b.Accent), 0.02f);
                    b.SetScale(shard, 0.0f);
                    b.To(shard, {ScaleT(1.0f, 0.3f), MoveT(d * r * 0.3f, d * r * (1.8f + Hash(i + 7)))}, t + 0.15f, 0.7f, "easeOutCubic");
                    b.To(shard, {One(TweenProperty::Rotation, 0.0f, 360.0f * (Hash(i) - 0.5f))}, t + 0.15f, 0.7f, "linear");
                    b.To(shard, {AlphaT(1.0f, 0.0f)}, t + 0.55f, 0.3f, "easeIn");
                }
            }
        }

        void ShapeLoop(MotionBuilder &b, const ShapeKind &kind, const std::string &style)
        {
            const float r = b.SizeOr(60.0f);
            const entt::entity e = b.Fill(kind.Path(r), {0, 0}, b.Color, 0.01f);
            b.Pop(e, 0.0f, 0.35f);
            if (style == "Pulse")
            {
                b.Glow(e, b.Color, 0.6f, 1.0f);
                b.To(e, {ScaleT(1.0f, 1.15f), One(TweenProperty::GlowIntensity, 0.6f, 1.8f)}, 0.4f, 0.45f, "easeInOutSine", 3, true);
            }
            else if (style == "Wobble")
            {
                b.To(e, {One(TweenProperty::Rotation, -14.0f, 14.0f)}, 0.4f, 0.3f, "easeInOutSine", 5, true);
            }
            else if (style == "Float")
            {
                b.To(e, {MoveT({0, 0}, {0.0f, r * 0.35f})}, 0.4f, 0.7f, "easeInOutSine", 3, true);
                const entt::entity shadow = b.Fill(VectorPath::Ellipse(r * 0.8f, r * 0.14f), {0.0f, -r * 1.3f}, WithA(glm::vec4(0, 0, 0, 1), 0.35f), 0.0f);
                b.To(shadow, {ScaleT(1.0f, 0.75f)}, 0.4f, 0.7f, "easeInOutSine", 3, true);
            }
        }

        std::vector<glm::vec2> CurvePoints(const std::function<float(float)> &fn, float x0, float x1, float w, float h, int n, glm::vec2 offset = {0.0f, 0.0f})
        {
            std::vector<glm::vec2> pts;
            for (int i = 0; i <= n; ++i)
            {
                const float u = static_cast<float>(i) / static_cast<float>(n);
                pts.push_back(offset + glm::vec2(-w * 0.5f + w * u, fn(x0 + (x1 - x0) * u) * h));
            }
            return pts;
        }

        void AddMore(std::vector<MotionPreset> &list)
        {
            static const struct
            {
                const char *Style;
                const char *Text;
                float Duration;
                bool Exit;
            } families[] = {
                {"PopOut", "swells a little and pops away", 0.85f, true},
                {"FadeOut", "fades away while drifting up", 0.85f, true},
                {"SpinOut", "spins away into nothing", 0.95f, true},
                {"Explode", "bursts into spinning shards", 1.2f, true},
                {"Pulse", "pops in, then pulses with a glow", 2.2f, false},
                {"Wobble", "pops in, then wobbles side to side", 2.2f, false},
                {"Float", "pops in, then floats over its shadow", 3.2f, false},
            };
            for (const ShapeKind &kind : ShapeKinds())
                for (const auto &family : families)
                {
                    const ShapeKind shape = kind;
                    const std::string style = family.Style;
                    const bool exit = family.Exit;
                    Add(list, std::string(kind.Name) + family.Style, exit ? "Exits" : "Loops", std::string("A ") + kind.Name + " " + family.Text, family.Duration,
                        [shape, style, exit](MotionBuilder &b)
                        {
                            if (exit)
                                ShapeExit(b, shape, style);
                            else
                                ShapeLoop(b, shape, style);
                        });
                }

            const auto wave = [](float x) { return 1.8f + 1.2f * std::sin(1.2f * x) + 0.25f * x; };

            Add(list, "SliceSplit", "Explain", "Rectangles under a curve split in half again and again (Riemann refinement)", 4.2f, [wave](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const float w = s * 2.4f, h = s * 0.32f;
                    const float base = -s * 0.8f;
                    b.Draw(b.Line({{-w * 0.5f - 10.0f, base}, {w * 0.5f + 10.0f, base}}, WithA(White, 0.7f), 2.5f), 0.0f, 0.4f);
                    const std::vector<glm::vec2> curve = CurvePoints(wave, 0.0f, 5.0f, w, h, 120, {0.0f, base});
                    b.Draw(b.Line(curve, Sky, 4.0f, 0.05f), 0.2f, 0.8f);
                    float t = 1.0f;
                    for (int level = 0; level < 4; ++level)
                    {
                        const int n = 4 << level;
                        const float bw = w / static_cast<float>(n);
                        const float show = t, hide = t + 0.8f;
                        for (int i = 0; i < n; ++i)
                        {
                            const float x = static_cast<float>(i) / static_cast<float>(n) * 5.0f;
                            const float height = wave(x) * h;
                            const float u = static_cast<float>(i) / static_cast<float>(n);
                            const entt::entity bar = b.Fill(Bar(bw * (level < 2 ? 0.92f : 0.98f), height, 0.0f), {-w * 0.5f + bw * (static_cast<float>(i) + 0.5f), base}, WithA(glm::mix(Orange, Gold, u), 0.7f), 0.0f);
                            b.Tf(bar).scale.y = 0.0f;
                            b.To(bar, {One(TweenProperty::ScaleY, 0.0f, 1.0f)}, show + 0.01f * static_cast<float>(i), 0.35f, "easeOutBack");
                            if (level < 3)
                                b.To(bar, {AlphaT(0.7f, 0.0f)}, hide, 0.25f, "easeIn");
                        }
                        char label[32];
                        std::snprintf(label, sizeof(label), "n = %d", n);
                        const entt::entity tag = b.Label(label, {w * 0.32f, s * 0.95f}, 26.0f, Gold, 0.1f);
                        b.FadeIn(tag, show, 0.2f);
                        if (level < 3)
                            b.To(tag, {AlphaT(1.0f, 0.0f)}, hide, 0.2f, "linear");
                        t += 0.85f;
                    } });
            Add(list, "AreaSweep", "Explain", "The area under a curve fills left to right behind a sweeping edge", 2.6f, [wave](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const float w = s * 2.4f, h = s * 0.32f, base = -s * 0.8f;
                    b.Draw(b.Line({{-w * 0.5f - 10.0f, base}, {w * 0.5f + 10.0f, base}}, WithA(White, 0.7f), 2.5f), 0.0f, 0.4f);
                    const std::vector<glm::vec2> curve = CurvePoints(wave, 0.0f, 5.0f, w, h, 100, {0.0f, base});
                    b.Draw(b.Line(curve, Sky, 4.0f, 0.05f), 0.1f, 0.8f);
                    const int strips = 50;
                    const float sw = w / static_cast<float>(strips);
                    for (int i = 0; i < strips; ++i)
                    {
                        const float x = (static_cast<float>(i) + 0.5f) / static_cast<float>(strips) * 5.0f;
                        const entt::entity strip = b.Fill(Bar(sw + 0.6f, wave(x) * h, 0.0f), {-w * 0.5f + sw * (static_cast<float>(i) + 0.5f), base}, WithA(Lime, 0.35f), 0.0f);
                        b.FadeIn(strip, 1.0f + 1.2f * static_cast<float>(i) / static_cast<float>(strips), 0.15f);
                    }
                    const entt::entity edge = b.Line({{0.0f, base}, {0.0f, base + h * 3.6f}}, Gold, 3.0f, 0.08f);
                    b.Fly(edge, {-w * 0.5f, 0.0f}, {w * 0.5f, 0.0f}, 1.0f, 1.2f, "linear");
                    b.SetAlpha(edge, 0.0f);
                    b.To(edge, {AlphaT(0.0f, 1.0f)}, 0.9f, 0.1f, "linear");
                    b.To(edge, {AlphaT(1.0f, 0.0f)}, 2.2f, 0.3f, "linear"); });
            Add(list, "TangentLine", "Explain", "A tangent line slides along a curve, tilting with the slope", 3.0f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const float w = s * 2.4f, amp = s * 0.5f;
                    const auto fn = [](float x) { return std::sin(x); };
                    const std::vector<glm::vec2> curve = CurvePoints(fn, -3.0f, 3.0f, w, amp, 120);
                    b.Draw(b.Line(curve, Sky, 4.0f), 0.0f, 0.8f);
                    const entt::entity tangent = b.Node("Tangent", curve.front(), 0.1f);
                    b.Line({{-s * 0.6f, 0.0f}, {s * 0.6f, 0.0f}}, Gold, 3.5f, 0.0f, false, tangent);
                    b.Fill(VectorPath::Circle(7.0f), {0, 0}, White, 0.01f, tangent);
                    b.Pop(tangent, 0.8f, 0.3f);
                    b.Along(tangent, curve, 1.0f, 1.8f, "easeInOutSine", true); });
            Add(list, "SecantTangent", "Explain", "A secant line closes in until it becomes the tangent", 2.6f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const float w = s * 2.4f, amp = s * 0.18f;
                    const auto fn = [](float x) { return x * x; };
                    const std::vector<glm::vec2> curve = CurvePoints(fn, -2.0f, 2.0f, w, amp, 100, {0.0f, -s * 0.5f});
                    b.Draw(b.Line(curve, Sky, 4.0f), 0.0f, 0.7f);
                    const glm::vec2 p{w * 0.125f, (0.5f * 0.5f) * amp - s * 0.5f};
                    const float slope = 2.0f * 0.5f * amp / (w / 4.0f);
                    const entt::entity line = b.Node("Secant", p, 0.1f);
                    b.Line({{-s * 1.1f, 0.0f}, {s * 1.1f, 0.0f}}, Gold, 3.0f, 0.0f, false, line);
                    b.Pop(b.Fill(VectorPath::Circle(7.0f), p, White, 0.2f), 0.6f, 0.3f);
                    const entt::entity q = b.Fill(VectorPath::Circle(7.0f), {w * 0.5f, 4.0f * amp - s * 0.5f}, Rose, 0.2f);
                    b.Pop(q, 0.7f, 0.3f);
                    const float a0 = Angle(glm::vec2(w * 0.5f, 4.0f * amp - s * 0.5f) - p);
                    b.Tf(line).rotation = a0;
                    b.FadeIn(line, 0.8f, 0.3f);
                    std::vector<glm::vec2> path;
                    for (int i = 0; i <= 30; ++i)
                    {
                        const float x = 2.0f - 1.5f * static_cast<float>(i) / 30.0f;
                        path.push_back({x / 4.0f * w, x * x * amp - s * 0.5f});
                    }
                    b.Along(q, path, 1.1f, 1.3f, "easeInOutCubic");
                    b.To(line, {One(TweenProperty::Rotation, a0, glm::degrees(std::atan(slope)))}, 1.1f, 1.3f, "easeInOutCubic"); });
            Add(list, "FunctionPen", "Explain", "A pen draws the curve with dashed guides to both axes", 2.4f, [wave](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const float w = s * 2.4f, h = s * 0.32f, base = -s * 0.8f;
                    Axes(b, {w, s * 1.6f}, base, 0.0f, false);
                    const std::vector<glm::vec2> curve = CurvePoints(wave, 0.0f, 5.0f, w, h, 120, {0.0f, base});
                    const entt::entity line = b.Line(curve, Sky, 4.0f, 0.05f);
                    b.Glow(line, Sky, 0.8f, 1.0f);
                    b.Draw(line, 0.5f, 1.6f, "easeInOutSine");
                    const entt::entity pen = b.Node("Pen", curve.front(), 0.2f);
                    b.Fill(VectorPath::Circle(8.0f), {0, 0}, White, 0.01f, pen);
                    b.Stroke(VectorPath::Circle(13.0f), {0, 0}, Sky, 3.0f, 0.0f, pen);
                    const entt::entity down = b.Line({{0.0f, 0.0f}, {0.0f, -s * 2.0f}}, WithA(White, 0.5f), 2.0f, -0.01f, false, pen);
                    b.V(down).dashLength = 8.0f;
                    b.V(down).dashGap = 7.0f;
                    b.Pop(pen, 0.4f, 0.2f);
                    b.Along(pen, curve, 0.5f, 1.6f, "easeInOutSine");
                    b.To(pen, {AlphaT(1.0f, 0.0f)}, 2.1f, 0.3f, "linear"); });
            Add(list, "SineFromCircle", "Explain", "A point circling a wheel traces a sine wave beside it", 3.4f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(70.0f);
                    const glm::vec2 c{-r * 2.0f, 0.0f};
                    b.Draw(b.Line(Arc(c, r, 0.0f, 360.0f, 64), WithA(White, 0.6f), 2.5f), 0.0f, 0.5f);
                    const entt::entity arm = b.Node("Arm", c, 0.1f);
                    b.Line({{0, 0}, {r, 0}}, Gold, 3.0f, 0.0f, false, arm);
                    b.Fill(VectorPath::Circle(7.0f), {r, 0.0f}, Gold, 0.01f, arm);
                    b.FadeIn(arm, 0.4f, 0.2f);
                    b.To(arm, {One(TweenProperty::Rotation, 0.0f, 720.0f)}, 0.5f, 2.6f, "linear");
                    std::vector<glm::vec2> wavePts;
                    for (int i = 0; i <= 160; ++i)
                    {
                        const float u = static_cast<float>(i) / 160.0f;
                        wavePts.push_back({-r * 0.8f + u * r * 4.6f, std::sin(u * 4.0f * Pi) * r});
                    }
                    b.Draw(b.Line(wavePts, Sky, 4.0f, 0.05f), 0.5f, 2.6f, "linear"); });
            Add(list, "VectorAdd", "Explain", "Two vectors placed tip to tail, then their sum", 2.4f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(120.0f);
                    const glm::vec2 a{s * 1.4f, s * 0.4f}, c{-s * 0.3f, s * 0.9f};
                    const glm::vec2 o{-s * 0.6f, -s * 0.6f};
                    const glm::vec4 keep = b.Color;
                    b.Color = Sky;
                    BuildArrow(b, {o, o + a}, false, false, false);
                    b.Color = keep;
                    const glm::vec4 rose = Rose;
                    const entt::entity body = b.Line({o + a, o + a + c}, rose, 7.0f);
                    b.Draw(body, 0.9f, 0.6f);
                    const entt::entity head = b.Fill(ArrowHead(18.0f), o + a + c, rose, 0.02f);
                    b.Tf(head).rotation = Angle(c);
                    b.Pop(head, 1.4f, 0.25f);
                    const entt::entity sum = b.Line({o, o + a + c}, Gold, 7.0f, 0.01f);
                    b.Glow(sum, Gold, 1.2f, 1.0f);
                    b.Draw(sum, 1.7f, 0.6f);
                    const entt::entity sumHead = b.Fill(ArrowHead(18.0f), o + a + c, Gold, 0.03f);
                    b.Tf(sumHead).rotation = Angle(a + c);
                    b.Pop(sumHead, 2.2f, 0.2f); });
            Add(list, "Pythagoras", "Explain", "A right triangle and the squares on its sides", 2.6f, [](MotionBuilder &b)
                {
                    const float k = b.SizeOr(26.0f);
                    const glm::vec2 p0{-1.5f * k, -1.0f * k}, p1{1.5f * k, -1.0f * k}, p2{1.5f * k, 3.0f * k};
                    const std::vector<glm::vec2> tri{p0, p1, p2};
                    b.Draw(b.Line(tri, White, 4.0f, 0.1f, true), 0.0f, 0.7f);
                    const auto square = [&](glm::vec2 a, glm::vec2 c, const glm::vec4 &color, float t)
                    {
                        const glm::vec2 d = c - a;
                        const glm::vec2 n{d.y, -d.x};
                        const entt::entity sq = b.Fill(VectorPath::Points({a, c, c + n, a + n}, true), {0, 0}, WithA(color, 0.6f), 0.0f);
                        b.FadeIn(sq, t, 0.4f);
                    };
                    square(p0, p1, Sky, 0.7f);
                    square(p1, p2, Lime, 1.0f);
                    square(p2, p0, Gold, 1.3f);
                    b.FadeIn(b.Label("a² + b² = c²", {0.0f, -4.6f * k}, 34.0f, White, 0.2f), 1.8f, 0.4f); });
            Add(list, "FractionBar", "Explain", "A bar splits into parts and some of them fill (values: parts, filled)", 2.0f, [](MotionBuilder &b)
                {
                    const std::vector<float> v = b.ValuesOr({5.0f, 3.0f});
                    const int parts = std::max(1, static_cast<int>(v[0]));
                    const int filled = v.size() > 1 ? static_cast<int>(v[1]) : parts / 2;
                    const float w = b.SizeOr(320.0f), h = w * 0.18f;
                    b.Draw(b.Stroke(VectorPath::Rect(w, h, 8.0f), {0, 0}, White, 3.0f, 0.1f), 0.0f, 0.6f);
                    const float cell = w / static_cast<float>(parts);
                    for (int i = 0; i < parts; ++i)
                    {
                        const float x = -w * 0.5f + cell * (static_cast<float>(i) + 0.5f);
                        if (i > 0)
                            b.Draw(b.Line({{x - cell * 0.5f, -h * 0.5f}, {x - cell * 0.5f, h * 0.5f}}, WithA(White, 0.7f), 2.0f, 0.1f), 0.5f + 0.06f * static_cast<float>(i), 0.2f);
                        if (i < filled)
                            b.Pop(b.Fill(VectorPath::Rect(cell - 8.0f, h - 8.0f, 4.0f), {x, 0.0f}, b.Color, 0.0f), 0.9f + 0.12f * static_cast<float>(i), 0.3f);
                    }
                    char label[32];
                    std::snprintf(label, sizeof(label), "%d / %d", filled, parts);
                    b.FadeIn(b.Label(label, {0.0f, -h - 30.0f}, 32.0f, White, 0.1f), 1.0f + 0.12f * static_cast<float>(filled), 0.3f); }, MotionInput::Values);
            Add(list, "Clock", "Explain", "A clock face draws and its hands sweep", 2.6f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(90.0f);
                    b.Draw(b.Stroke(VectorPath::Circle(r), {0, 0}, White, 4.0f), 0.0f, 0.6f);
                    for (int i = 0; i < 12; ++i)
                    {
                        const glm::vec2 d = Dir(90.0f - 30.0f * static_cast<float>(i));
                        b.Pop(b.Line({d * r * 0.82f, d * r * 0.94f}, WithA(White, 0.8f), i % 3 == 0 ? 4.0f : 2.0f), 0.3f + 0.03f * static_cast<float>(i), 0.2f);
                    }
                    const entt::entity minute = b.Node("Minute", {0, 0}, 0.1f);
                    b.Line({{0, 0}, {0.0f, r * 0.78f}}, b.Color, 4.0f, 0.0f, false, minute);
                    const entt::entity hour = b.Node("Hour", {0, 0}, 0.1f);
                    b.Line({{0, 0}, {0.0f, r * 0.5f}}, b.Accent, 6.0f, 0.0f, false, hour);
                    b.Pop(b.Fill(VectorPath::Circle(6.0f), {0, 0}, White, 0.2f), 0.7f, 0.2f);
                    b.FadeIn(minute, 0.7f, 0.2f);
                    b.FadeIn(hour, 0.7f, 0.2f);
                    b.To(minute, {One(TweenProperty::Rotation, 0.0f, -720.0f)}, 0.8f, 1.8f, "easeInOutCubic");
                    b.To(hour, {One(TweenProperty::Rotation, 0.0f, -60.0f)}, 0.8f, 1.8f, "easeInOutCubic"); });
            Add(list, "MatrixGrid", "Explain", "A grid of cells pops in along its diagonals", 1.6f, [](MotionBuilder &b)
                {
                    const int n = 6;
                    const float cell = b.SizeOr(34.0f);
                    for (int y = 0; y < n; ++y)
                        for (int x = 0; x < n; ++x)
                        {
                            const glm::vec2 p{(static_cast<float>(x) - 2.5f) * cell * 1.15f, (2.5f - static_cast<float>(y)) * cell * 1.15f};
                            const float u = static_cast<float>(x + y) / static_cast<float>(2 * n - 2);
                            b.Pop(b.Fill(VectorPath::Rect(cell, cell, 5.0f), p, glm::mix(b.Color, b.Accent, u)), 0.06f * static_cast<float>(x + y), 0.35f);
                        } });
            Add(list, "TreeGrow", "Explain", "A tree grows branch by branch", 2.4f, [](MotionBuilder &b)
                {
                    const float len = b.SizeOr(90.0f);
                    std::function<void(glm::vec2, float, float, int, float)> grow = [&](glm::vec2 from, float angle, float l, int depth, float t)
                    {
                        const glm::vec2 to = from + Dir(angle) * l;
                        const entt::entity branch = b.Line({from, to}, glm::mix(Orange, Lime, static_cast<float>(5 - depth) / 5.0f), std::max(1.5f, static_cast<float>(depth) * 1.8f));
                        b.Draw(branch, t, 0.35f, "easeOut");
                        if (depth <= 1)
                        {
                            b.Pop(b.Fill(VectorPath::Circle(6.0f), to, Lime, 0.1f), t + 0.3f, 0.3f);
                            return;
                        }
                        grow(to, angle + 24.0f, l * 0.72f, depth - 1, t + 0.32f);
                        grow(to, angle - 24.0f, l * 0.72f, depth - 1, t + 0.32f);
                    };
                    grow({0.0f, -len * 1.5f}, 90.0f, len, 5, 0.0f); });
            Add(list, "NetworkGraph", "Explain", "Nodes pop in and edges connect them", 2.0f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(130.0f);
                    std::vector<glm::vec2> nodes;
                    for (int i = 0; i < 7; ++i)
                        nodes.push_back(i == 0 ? glm::vec2(0.0f) : Dir(60.0f * static_cast<float>(i) + 15.0f * Hash(i)) * r * (0.7f + 0.3f * Hash(i + 3)));
                    static const int edges[][2] = {{0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {1, 2}, {3, 4}, {5, 6}, {2, 3}};
                    int k = 0;
                    for (const auto &e : edges)
                        b.Draw(b.Line({nodes[static_cast<size_t>(e[0])], nodes[static_cast<size_t>(e[1])]}, WithA(White, 0.45f), 2.5f, 0.0f), 0.5f + 0.08f * static_cast<float>(k++), 0.35f);
                    for (size_t i = 0; i < nodes.size(); ++i)
                    {
                        const entt::entity node = b.Fill(VectorPath::Circle(i == 0 ? 18.0f : 12.0f), nodes[i], PaletteColor(i, b.Color, b.Accent), 0.1f);
                        b.Pop(node, 0.06f * static_cast<float>(i), 0.35f);
                    } });
            Add(list, "FlowChart", "Explain", "Boxes appear in a row, joined by arrows", 2.2f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(110.0f);
                    for (int i = 0; i < 3; ++i)
                    {
                        const float x = (static_cast<float>(i) - 1.0f) * w * 1.6f;
                        const float t = 0.6f * static_cast<float>(i);
                        b.Pop(b.Fill(VectorPath::Rect(w, w * 0.55f, 12.0f), {x, 0.0f}, PaletteColor(i, b.Color, b.Accent)), t, 0.4f);
                        static const char *labels[] = {"Input", "Process", "Output"};
                        b.Pop(b.Label(labels[i], {x, 0.0f}, w * 0.2f, glm::vec4(0.05f, 0.07f, 0.1f, 1.0f), 0.1f), t + 0.15f, 0.3f);
                        if (i < 2)
                        {
                            const entt::entity a = b.Line({{x + w * 0.55f, 0.0f}, {x + w * 1.0f, 0.0f}}, White, 4.0f);
                            b.Draw(a, t + 0.35f, 0.25f);
                            const entt::entity head = b.Fill(ArrowHead(11.0f), {x + w * 1.05f, 0.0f}, White, 0.02f);
                            b.Pop(head, t + 0.55f, 0.2f);
                        }
                    } });
            Add(list, "Timeline", "Explain", "A line draws and milestones pop along it", 2.2f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(380.0f);
                    b.Draw(b.Line({{-w * 0.5f, 0.0f}, {w * 0.5f, 0.0f}}, WithA(White, 0.7f), 4.0f), 0.0f, 1.2f, "easeInOut");
                    for (int i = 0; i < 5; ++i)
                    {
                        const float x = -w * 0.5f + w * (static_cast<float>(i) + 0.5f) / 5.0f;
                        const float t = 0.2f + 0.24f * static_cast<float>(i);
                        b.Pop(b.Fill(VectorPath::Circle(10.0f), {x, 0.0f}, PaletteColor(i, b.Color, b.Accent), 0.1f), t, 0.3f);
                        const entt::entity tick = b.Line({{x, 14.0f}, {x, i % 2 ? -46.0f : 46.0f}}, WithA(White, 0.4f), 2.0f);
                        b.Draw(tick, t + 0.15f, 0.2f);
                        char label[16];
                        std::snprintf(label, sizeof(label), "%d", 2020 + i);
                        b.FadeIn(b.Label(label, {x, i % 2 ? -66.0f : 66.0f}, 22.0f, White, 0.1f), t + 0.3f, 0.3f);
                    } });
            Add(list, "Venn", "Explain", "Two circles slide together and overlap", 1.8f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(80.0f);
                    const entt::entity a = b.Fill(VectorPath::Circle(r), {-r * 2.0f, 0.0f}, WithA(Sky, 0.55f));
                    const entt::entity c = b.Fill(VectorPath::Circle(r), {r * 2.0f, 0.0f}, WithA(Rose, 0.55f), 0.01f);
                    b.Pop(a, 0.0f, 0.4f);
                    b.Pop(c, 0.15f, 0.4f);
                    b.To(a, {MoveT({-r * 2.0f, 0.0f}, {-r * 0.55f, 0.0f})}, 0.6f, 0.8f, "easeInOutCubic");
                    b.To(c, {MoveT({r * 2.0f, 0.0f}, {r * 0.55f, 0.0f})}, 0.6f, 0.8f, "easeInOutCubic");
                    b.Draw(b.Stroke(VectorPath::Circle(r), {-r * 0.55f, 0.0f}, White, 2.0f, 0.02f), 1.3f, 0.4f);
                    b.Draw(b.Stroke(VectorPath::Circle(r), {r * 0.55f, 0.0f}, White, 2.0f, 0.02f), 1.3f, 0.4f); });
            const auto traced = [](MotionBuilder &b, const std::function<glm::vec2(float)> &fn, float d)
            {
                std::vector<glm::vec2> pts;
                for (int i = 0; i <= 400; ++i)
                    pts.push_back(fn(static_cast<float>(i) / 400.0f));
                const entt::entity line = b.Line(pts, b.Color, b.WidthOr(3.5f));
                b.Glow(line, b.Color, 0.9f, 1.0f);
                b.Draw(line, 0.0f, d, "easeInOut");
                const entt::entity dot = b.Fill(VectorPath::Circle(6.0f), pts.front(), White, 0.02f);
                b.Along(dot, pts, 0.0f, d, "easeInOut");
            };
            Add(list, "Lissajous", "Explain", "A Lissajous figure traces itself", 2.4f, [traced](MotionBuilder &b)
                {
                    const float r = b.SizeOr(110.0f);
                    traced(b, [r](float u) { const float t = u * 2.0f * Pi; return glm::vec2(std::sin(3.0f * t + Pi * 0.5f), std::sin(2.0f * t)) * r; }, 2.2f); });
            Add(list, "Spirograph", "Explain", "A spirograph loop draws round and round", 3.0f, [traced](MotionBuilder &b)
                {
                    const float r = b.SizeOr(110.0f);
                    traced(b, [r](float u)
                           {
                               const float t = u * 6.0f * Pi;
                               const float R = 1.0f, k = 0.38f, l = 0.75f;
                               return glm::vec2((R - k) * std::cos(t) + l * k * std::cos((R - k) / k * t), (R - k) * std::sin(t) - l * k * std::sin((R - k) / k * t)) * r * 1.4f;
                           }, 2.8f); });
            Add(list, "RoseCurve", "Explain", "A five-petal rose curve traces itself", 2.4f, [traced](MotionBuilder &b)
                {
                    const float r = b.SizeOr(110.0f);
                    traced(b, [r](float u) { const float t = u * Pi; const float k = std::cos(5.0f * t); return glm::vec2(std::cos(t), std::sin(t)) * k * r; }, 2.2f); });
            Add(list, "Projectile", "Explain", "A ball flies along a parabola, leaving its path", 1.8f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(150.0f);
                    std::vector<glm::vec2> pts;
                    for (int i = 0; i <= 60; ++i)
                    {
                        const float u = static_cast<float>(i) / 60.0f;
                        pts.push_back({-s * 1.3f + u * s * 2.6f, -s * 0.6f + 4.0f * u * (1.0f - u) * s * 1.2f});
                    }
                    b.Draw(b.Line({{-s * 1.4f, -s * 0.6f}, {s * 1.4f, -s * 0.6f}}, WithA(White, 0.5f), 2.0f), 0.0f, 0.3f);
                    const entt::entity path = b.Line(pts, WithA(White, 0.5f), 2.5f, 0.0f);
                    b.V(path).dashLength = 10.0f;
                    b.V(path).dashGap = 8.0f;
                    b.Draw(path, 0.3f, 1.2f, "linear");
                    const entt::entity ball = b.Fill(VectorPath::Circle(12.0f), pts.front(), b.Accent, 0.1f);
                    b.Along(ball, pts, 0.3f, 1.2f, "linear");
                    b.Pop(b.Stroke(VectorPath::Circle(14.0f), pts.back(), b.Accent, 3.0f), 1.5f, 0.3f); });
            Add(list, "PointPlot", "Explain", "A point appears with dashed guides and its (x, y) label", 1.4f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    Axes(b, {s * 2.4f, s * 1.6f}, -s * 0.8f, 0.0f, false);
                    const glm::vec2 p = b.O.Points.empty() ? glm::vec2(s * 0.5f, s * 0.3f) : b.O.Points.back();
                    const glm::vec2 o{-s * 1.2f, -s * 0.8f};
                    const entt::entity v = b.Line({{p.x, o.y}, p}, WithA(White, 0.6f), 2.0f);
                    const entt::entity h = b.Line({{o.x, p.y}, p}, WithA(White, 0.6f), 2.0f);
                    b.V(v).dashLength = b.V(h).dashLength = 8.0f;
                    b.V(v).dashGap = b.V(h).dashGap = 6.0f;
                    b.Draw(v, 0.5f, 0.4f);
                    b.Draw(h, 0.5f, 0.4f);
                    const entt::entity dot = b.Fill(VectorPath::Circle(9.0f), p, b.Accent, 0.1f);
                    b.Glow(dot, b.Accent, 1.4f, 1.0f);
                    b.Pop(dot, 0.8f, 0.3f);
                    b.Pop(b.Label(b.TextOr("(3, 2)"), p + glm::vec2(48.0f, 28.0f), 26.0f, White, 0.1f), 1.0f, 0.3f); }, MotionInput::Points);

            Add(list, "StackedBars", "Charts", "Stacked bars build up layer by layer (values: one column per three)", 1.8f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const Chart c{s * 2.4f, s * 1.6f};
                    std::vector<float> values = b.ValuesOr({2, 1, 1, 3, 2, 1, 1, 2, 2, 2, 3, 1});
                    const int columns = std::max(1, static_cast<int>(values.size()) / 3);
                    float hi = 0.0f;
                    for (int col = 0; col < columns; ++col)
                        hi = std::max(hi, values[static_cast<size_t>(col * 3)] + values[static_cast<size_t>(col * 3 + 1)] + values[static_cast<size_t>(col * 3 + 2)]);
                    Axes(b, c, -c.H * 0.5f, 0.0f, false);
                    const float slot = c.W / static_cast<float>(columns);
                    for (int layer = 0; layer < 3; ++layer)
                        for (int col = 0; col < columns; ++col)
                        {
                            float below = 0.0f;
                            for (int l = 0; l < layer; ++l)
                                below += values[static_cast<size_t>(col * 3 + l)];
                            const float h = values[static_cast<size_t>(col * 3 + layer)] / hi * c.H * 0.9f;
                            const glm::vec2 base{-c.W * 0.5f + slot * (static_cast<float>(col) + 0.5f), -c.H * 0.5f + below / hi * c.H * 0.9f};
                            const entt::entity bar = b.Fill(Bar(slot * 0.6f, h, 0.0f), base, PaletteColor(static_cast<size_t>(layer), b.Color, b.Accent), 0.0f);
                            b.Tf(bar).scale.y = 0.0f;
                            b.To(bar, {One(TweenProperty::ScaleY, 0.0f, 1.0f)}, 0.4f + 0.4f * static_cast<float>(layer) + 0.06f * static_cast<float>(col), 0.4f, "easeOutCubic");
                        } }, MotionInput::Values);
            Add(list, "RadarChart", "Charts", "A spider chart: web, then the value shape grows out (values 0..1)", 1.8f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(110.0f);
                    std::vector<float> values = b.ValuesOr({0.8f, 0.55f, 0.9f, 0.4f, 0.7f, 0.6f});
                    const size_t n = std::max<size_t>(3, values.size());
                    for (int ring = 1; ring <= 4; ++ring)
                    {
                        std::vector<glm::vec2> pts;
                        for (size_t i = 0; i < n; ++i)
                            pts.push_back(Dir(90.0f + 360.0f * static_cast<float>(i) / static_cast<float>(n)) * r * static_cast<float>(ring) / 4.0f);
                        b.Draw(b.Line(pts, WithA(White, 0.25f), 1.5f, -0.01f, true), 0.08f * static_cast<float>(ring), 0.4f);
                    }
                    std::vector<glm::vec2> shape;
                    for (size_t i = 0; i < n; ++i)
                    {
                        const glm::vec2 d = Dir(90.0f + 360.0f * static_cast<float>(i) / static_cast<float>(n));
                        b.Draw(b.Line({{0, 0}, d * r}, WithA(White, 0.25f), 1.5f, -0.01f), 0.3f, 0.3f);
                        shape.push_back(d * r * std::clamp(i < values.size() ? values[i] : 0.0f, 0.0f, 1.0f));
                    }
                    const entt::entity fill = b.Fill(VectorPath::Points(shape, true), {0, 0}, WithA(b.Color, 0.45f), 0.0f);
                    b.Pop(fill, 0.7f, 0.7f, "easeOutBack");
                    const entt::entity edge = b.Line(shape, b.Color, 3.0f, 0.01f, true);
                    b.Pop(edge, 0.7f, 0.7f, "easeOutBack"); }, MotionInput::Values);
            Add(list, "PercentPie", "Charts", "A pie wedge sweeps to a percentage (values[1] is 0..1)", 1.6f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(90.0f);
                    const float value = std::clamp(b.ValuesOr({0.68f})[0], 0.0f, 1.0f);
                    b.FadeIn(b.Fill(VectorPath::Circle(r), {0, 0}, WithA(White, 0.1f), -0.01f), 0.0f, 0.3f);
                    const entt::entity wedge = b.Line(Arc({0, 0}, r * 0.5f, 90.0f, 90.0f - 360.0f * value, 96), b.Color, r);
                    b.V(wedge).cap = StrokeCap::Butt;
                    b.Draw(wedge, 0.2f, 1.1f, "easeInOutCubic");
                    const entt::entity label = b.Label("0%", {0.0f, -r - 30.0f}, 30.0f, White, 0.1f);
                    b.Value(0.2f, 1.1f, "easeInOutCubic", [&reg = b.R, label, value](float v)
                            {
                                if (!reg.valid(label))
                                    return;
                                char t[16];
                                std::snprintf(t, sizeof(t), "%d%%", static_cast<int>(std::round(v * value * 100.0f)));
                                reg.get<TextComponent>(label).Text = t;
                            }); }, MotionInput::Values);
            Add(list, "SortBars", "Charts", "Bars swap places until they are sorted", 3.2f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    std::vector<float> values = b.ValuesOr({5, 2, 8, 3, 6, 1, 7, 4});
                    const size_t n = values.size();
                    float hi = *std::max_element(values.begin(), values.end());
                    hi = std::max(hi, 1e-4f);
                    const float w = s * 2.4f, slot = w / static_cast<float>(n);
                    std::vector<entt::entity> bars;
                    std::vector<int> slotOf(n);
                    for (size_t i = 0; i < n; ++i)
                    {
                        const entt::entity bar = b.Fill(Bar(slot * 0.7f, values[i] / hi * s * 1.4f, 4.0f), {-w * 0.5f + slot * (static_cast<float>(i) + 0.5f), -s * 0.7f}, glm::mix(b.Color, b.Accent, values[i] / hi));
                        b.Tf(bar).scale.y = 0.0f;
                        b.To(bar, {One(TweenProperty::ScaleY, 0.0f, 1.0f)}, 0.05f * static_cast<float>(i), 0.35f, "easeOutBack");
                        bars.push_back(bar);
                        slotOf[i] = static_cast<int>(i);
                    }
                    std::vector<size_t> order(n);
                    for (size_t i = 0; i < n; ++i)
                        order[i] = i;
                    float t = 0.7f;
                    for (size_t pass = 0; pass < n; ++pass)
                        for (size_t j = 0; j + 1 < n - pass; ++j)
                            if (values[order[j]] > values[order[j + 1]])
                            {
                                const size_t a = order[j], c = order[j + 1];
                                const glm::vec2 pa{-w * 0.5f + slot * (static_cast<float>(j) + 0.5f), -s * 0.7f};
                                const glm::vec2 pc{-w * 0.5f + slot * (static_cast<float>(j + 1) + 0.5f), -s * 0.7f};
                                b.To(bars[a], {MoveT(pa, pc)}, t, 0.16f, "easeInOutCubic");
                                b.To(bars[c], {MoveT(pc, pa)}, t, 0.16f, "easeInOutCubic");
                                std::swap(order[j], order[j + 1]);
                                t += 0.18f;
                            } }, MotionInput::Values);
            Add(list, "Equalizer", "Charts", "Audio equalizer bars bounce", 2.4f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(120.0f);
                    for (int i = 0; i < 12; ++i)
                    {
                        const glm::vec2 base{(static_cast<float>(i) - 5.5f) * s * 0.22f, -s * 0.6f};
                        const entt::entity bar = b.Fill(Bar(s * 0.15f, s * 1.2f, 4.0f), base, glm::mix(b.Color, b.Accent, static_cast<float>(i) / 11.0f));
                        const float lo = 0.15f + 0.2f * Hash(i), hi = 0.55f + 0.45f * Hash(i + 20);
                        b.Tf(bar).scale.y = lo;
                        b.To(bar, {One(TweenProperty::ScaleY, lo, hi)}, 0.0f, 0.18f + 0.12f * Hash(i + 40), "easeInOutSine", 7, true);
                    } });

            Add(list, "BouncingBall", "Physics", "A ball drops and bounces, squashing on each hit", 2.4f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(26.0f);
                    const float floor = -r * 4.0f;
                    b.Draw(b.Line({{-r * 6.0f, floor - r}, {r * 6.0f, floor - r}}, WithA(White, 0.5f), 3.0f), 0.0f, 0.3f);
                    const entt::entity ball = b.Fill(VectorPath::Circle(r), {0.0f, r * 5.0f}, b.Accent, 0.1f);
                    float t = 0.0f, height = r * 9.0f;
                    for (int i = 0; i < 4; ++i)
                    {
                        const float fall = 0.18f * std::sqrt(height / r);
                        b.To(ball, {MoveT({0.0f, floor + height}, {0.0f, floor})}, t, fall, "easeInQuad");
                        b.To(ball, {Two(TweenProperty::Scale, {1.0f, 1.0f}, {1.3f, 0.7f})}, t + fall, 0.06f, "easeOut");
                        b.To(ball, {Two(TweenProperty::Scale, {1.3f, 0.7f}, {1.0f, 1.0f})}, t + fall + 0.06f, 0.1f, "easeOut");
                        height *= 0.5f;
                        const float rise = 0.18f * std::sqrt(height / r);
                        b.To(ball, {MoveT({0.0f, floor}, {0.0f, floor + height})}, t + fall, rise, "easeOutQuad");
                        t += fall + rise;
                    } });
            Add(list, "SpringMass", "Physics", "A weight bobs on a coil spring", 2.6f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(100.0f);
                    b.Line({{-s * 0.5f, s}, {s * 0.5f, s}}, White, 4.0f);
                    std::vector<glm::vec2> coil;
                    for (int i = 0; i <= 20; ++i)
                    {
                        const float u = static_cast<float>(i) / 20.0f;
                        coil.push_back({(i == 0 || i == 20) ? 0.0f : (i % 2 ? -s * 0.2f : s * 0.2f), -u * s});
                    }
                    const entt::entity spring = b.Line(coil, WithA(White, 0.8f), 3.0f);
                    b.Tf(spring).position = {0.0f, s};
                    const entt::entity weight = b.Fill(VectorPath::Rect(s * 0.6f, s * 0.4f, 6.0f), {0.0f, -s * 0.2f}, b.Accent, 0.1f);
                    b.To(weight, {MoveT({0.0f, -s * 0.2f}, {0.0f, -s * 0.75f})}, 0.0f, 0.4f, "easeInOutSine", 5, true);
                    b.To(spring, {Two(TweenProperty::Scale, {1.0f, 1.0f}, {1.0f, 1.55f})}, 0.0f, 0.4f, "easeInOutSine", 5, true); });

            Add(list, "Toast", "Game UI", "A notification slides down, waits and slides away (text)", 2.4f, [](MotionBuilder &b)
                {
                    const std::string text = b.TextOr("Achievement unlocked");
                    const float w = b.SizeOr(360.0f), h = w * 0.2f;
                    const entt::entity panel = b.Node("Toast", {0.0f, h * 3.0f});
                    b.Fill(VectorPath::Rect(w, h, h * 0.3f), {0, 0}, glm::vec4(0.13f, 0.16f, 0.24f, 1.0f), 0.0f, panel);
                    b.Fill(VectorPath::Circle(h * 0.28f), {-w * 0.5f + h * 0.55f, 0.0f}, b.Accent, 0.01f, panel);
                    b.Label(text, {-w * 0.5f + h * 1.05f, 0.0f}, h * 0.32f, White, 0.02f, 0, panel);
                    b.Fly(panel, {0.0f, h * 3.0f}, {0.0f, 0.0f}, 0.0f, 0.45f, "easeOutBack");
                    b.Fly(panel, {0.0f, 0.0f}, {0.0f, h * 3.0f}, 1.8f, 0.4f, "easeInCubic"); }, MotionInput::Text);
            Add(list, "Tooltip", "Game UI", "A tooltip bubble pops up from a point (text)", 1.0f, [](MotionBuilder &b)
                {
                    const std::string text = b.TextOr("Press E");
                    const float h = b.SizeOr(54.0f);
                    const float w = std::max(h * 2.0f, h * 0.3f * static_cast<float>(text.size()) + h * 0.6f);
                    const entt::entity tip = b.Node("Tip", {0.0f, 0.0f});
                    b.Fill(VectorPath::Rect(w, h, h * 0.3f), {0.0f, h * 0.9f}, White, 0.0f, tip);
                    b.Fill(VectorPath::Points({{-h * 0.18f, h * 0.42f}, {h * 0.18f, h * 0.42f}, {0.0f, h * 0.12f}}, true), {0, 0}, White, 0.0f, tip);
                    b.Label(text, {0.0f, h * 0.9f}, h * 0.42f, glm::vec4(0.08f, 0.1f, 0.15f, 1.0f), 0.01f, 1, tip);
                    b.Pop(tip, 0.0f, 0.45f); }, MotionInput::Text);
            Add(list, "ButtonPress", "Game UI", "A button pops in, gets pressed and ripples", 1.4f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(180.0f), h = w * 0.32f;
                    const entt::entity button = b.Node("Button", {0, 0});
                    b.Fill(VectorPath::Rect(w, h, h * 0.5f), {0, 0}, b.Color, 0.0f, button);
                    b.Label(b.TextOr("Play"), {0, 0}, h * 0.45f, glm::vec4(0.05f, 0.07f, 0.1f, 1.0f), 0.01f, 1, button);
                    b.Pop(button, 0.0f, 0.45f);
                    b.To(button, {ScaleT(1.0f, 0.88f)}, 0.7f, 0.08f, "easeOut");
                    b.To(button, {ScaleT(0.88f, 1.0f)}, 0.78f, 0.3f, "easeOutBack");
                    const entt::entity ripple = b.Stroke(VectorPath::Rect(w, h, h * 0.5f), {0, 0}, b.Color, 3.0f, -0.01f);
                    b.SetAlpha(ripple, 0.0f);
                    b.To(ripple, {ScaleT(1.0f, 1.4f), AlphaT(0.9f, 0.0f)}, 0.75f, 0.5f, "easeOut"); }, MotionInput::Text);
            Add(list, "Slider", "Game UI", "A slider knob slides to a value and the track fills (values[1] is 0..1)", 1.2f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(280.0f);
                    const float value = std::clamp(b.ValuesOr({0.7f})[0], 0.0f, 1.0f);
                    b.FadeIn(b.Line({{-w * 0.5f, 0.0f}, {w * 0.5f, 0.0f}}, WithA(White, 0.2f), 10.0f, -0.01f), 0.0f, 0.3f);
                    const entt::entity fill = b.Line({{-w * 0.5f, 0.0f}, {w * 0.5f, 0.0f}}, b.Color, 10.0f);
                    b.SetTrim(fill, 0.0f, 0.0f);
                    b.To(fill, {One(TweenProperty::TrimEnd, 0.0f, value)}, 0.2f, 0.8f, "easeInOutCubic");
                    const entt::entity knob = b.Fill(VectorPath::Circle(16.0f), {-w * 0.5f, 0.0f}, White, 0.02f);
                    b.Fly(knob, {-w * 0.5f, 0.0f}, {-w * 0.5f + w * value, 0.0f}, 0.2f, 0.8f, "easeInOutCubic"); }, MotionInput::Values);
            Add(list, "StarRating", "Game UI", "Five stars pop in and fill up to a rating (values[1] is 0..5)", 1.6f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(26.0f);
                    const int rating = std::clamp(static_cast<int>(std::round(b.ValuesOr({4.0f})[0])), 0, 5);
                    for (int i = 0; i < 5; ++i)
                    {
                        const glm::vec2 p{(static_cast<float>(i) - 2.0f) * r * 2.5f, 0.0f};
                        b.Pop(b.Stroke(VectorPath::Star(5, r, r * 0.45f), p, WithA(White, 0.5f), 2.5f, -0.01f), 0.06f * static_cast<float>(i), 0.3f);
                        if (i < rating)
                        {
                            const entt::entity star = b.Fill(VectorPath::Star(5, r, r * 0.45f), p, Gold, 0.01f);
                            b.Glow(star, Gold, 1.0f, 0.8f);
                            b.Pop(star, 0.5f + 0.15f * static_cast<float>(i), 0.4f);
                        }
                    } }, MotionInput::Values);
            Add(list, "LikeBurst", "Game UI", "A heart pops with a burst of little dots", 1.2f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(46.0f);
                    const entt::entity ring = b.Stroke(VectorPath::Circle(r), {0, 0}, Violet, 8.0f);
                    b.SetScale(ring, 0.0f);
                    b.To(ring, {ScaleT(0.2f, 1.6f), One(TweenProperty::StrokeWidth, 10.0f, 0.5f)}, 0.0f, 0.4f, "easeOut");
                    const entt::entity heart = b.Fill(VectorPath::Heart(r * 1.8f), {0, 0}, b.ColorOr(Rose), 0.01f);
                    b.Pop(heart, 0.25f, 0.6f, "elasticOut");
                    for (int i = 0; i < 8; ++i)
                    {
                        const glm::vec2 d = Dir(45.0f * static_cast<float>(i));
                        const entt::entity dot = b.Fill(VectorPath::Circle(5.0f), d * r * 1.2f, PaletteColor(i % 5, b.Accent, Sky), 0.02f);
                        b.SetScale(dot, 0.0f);
                        b.To(dot, {ScaleT(1.0f, 0.0f), MoveT(d * r * 1.2f, d * r * 2.0f)}, 0.3f, 0.5f, "easeOut");
                    } });
            Add(list, "CoinCollect", "Game UI", "A coin flips, flies up and shows +1", 1.2f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(26.0f);
                    const entt::entity coin = b.Fill(VectorPath::Circle(r), {0, 0}, Gold, 0.1f);
                    b.Glow(coin, Gold, 1.2f, 0.8f);
                    b.To(coin, {Two(TweenProperty::Scale, {1.0f, 1.0f}, {0.1f, 1.0f})}, 0.0f, 0.12f, "easeInOutSine", 5, true);
                    b.To(coin, {MoveT({0, 0}, {0.0f, r * 4.0f})}, 0.0f, 0.7f, "easeOutCubic");
                    b.To(coin, {AlphaT(1.0f, 0.0f)}, 0.55f, 0.2f, "easeIn");
                    const entt::entity plus = b.Label("+1", {0.0f, r * 3.0f}, r * 1.2f, Gold, 0.2f);
                    b.Pop(plus, 0.55f, 0.3f);
                    b.Fly(plus, {0.0f, r * 3.0f}, {0.0f, r * 4.5f}, 0.55f, 0.6f);
                    b.To(plus, {AlphaT(1.0f, 0.0f)}, 0.9f, 0.3f, "easeIn"); });
            Add(list, "XPBar", "Game UI", "An XP bar fills, flashes and shows LEVEL UP", 2.0f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(300.0f), h = 18.0f;
                    b.FadeIn(b.Fill(VectorPath::Rect(w, h, h * 0.5f), {0, 0}, WithA(White, 0.12f), -0.01f), 0.0f, 0.2f);
                    const entt::entity fill = b.Line({{-w * 0.5f + h * 0.5f, 0.0f}, {w * 0.5f - h * 0.5f, 0.0f}}, Violet, h);
                    b.Glow(fill, Violet, 0.0f, 1.0f);
                    b.SetTrim(fill, 0.0f, 0.0f);
                    b.To(fill, {One(TweenProperty::TrimEnd, 0.0f, 1.0f)}, 0.1f, 1.0f, "easeInCubic");
                    b.To(fill, {One(TweenProperty::GlowIntensity, 0.0f, 3.0f)}, 1.1f, 0.15f, "easeOut");
                    b.To(fill, {One(TweenProperty::GlowIntensity, 3.0f, 0.8f)}, 1.25f, 0.5f, "easeIn");
                    const entt::entity text = b.Label("LEVEL UP", {0.0f, 44.0f}, 34.0f, Gold, 0.1f);
                    b.Glow(text, Gold, 1.0f, 1.0f);
                    b.Pop(text, 1.15f, 0.45f, "elasticOut"); });
            Add(list, "HealthDrain", "Game UI", "A health bar takes a hit: the red drops at once, the white chunk catches up", 1.4f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(300.0f), h = 18.0f;
                    const float after = std::clamp(b.ValuesOr({0.55f})[0], 0.0f, 1.0f);
                    b.Fill(VectorPath::Rect(w, h, h * 0.5f), {0, 0}, WithA(White, 0.12f), -0.02f);
                    const std::vector<glm::vec2> span{{-w * 0.5f + h * 0.5f, 0.0f}, {w * 0.5f - h * 0.5f, 0.0f}};
                    const entt::entity chunk = b.Line(span, White, h, -0.01f);
                    const entt::entity bar = b.Line(span, Rose, h);
                    b.To(bar, {One(TweenProperty::TrimEnd, 1.0f, after)}, 0.3f, 0.08f, "easeOut");
                    b.To(chunk, {One(TweenProperty::TrimEnd, 1.0f, after)}, 0.7f, 0.5f, "easeInOutCubic"); }, MotionInput::Values);
            Add(list, "Countdown", "Game UI", "3, 2, 1, GO!", 3.6f, [](MotionBuilder &b)
                {
                    const float size = b.SizeOr(110.0f);
                    static const char *steps[] = {"3", "2", "1", "GO!"};
                    for (int i = 0; i < 4; ++i)
                    {
                        const float t = 0.85f * static_cast<float>(i);
                        const glm::vec4 c = i == 3 ? Lime : b.ColorOr(White);
                        const entt::entity e = b.Label(steps[i], {0, 0}, size, c, 0.1f);
                        if (i == 3)
                            b.Glow(e, Lime, 1.4f, 1.0f);
                        b.SetAlpha(e, 0.0f);
                        b.To(e, {ScaleT(1.8f, 1.0f), AlphaT(0.0f, 1.0f)}, t, 0.3f, "easeOutCubic");
                        b.To(e, {ScaleT(1.0f, 0.6f), AlphaT(1.0f, 0.0f)}, t + 0.55f, 0.3f, "easeInCubic");
                    } });
            Add(list, "TimerRing", "Game UI", "A ring counts down and turns red near the end", 3.2f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(60.0f);
                    b.Fill(VectorPath::Circle(r), {0, 0}, WithA(White, 0.06f), -0.02f);
                    const entt::entity ring = b.Line(Arc({0, 0}, r, 90.0f, -270.0f, 96), b.Color, 9.0f);
                    b.To(ring, {One(TweenProperty::TrimStart, 0.0f, 1.0f)}, 0.0f, 3.0f, "linear");
                    const SDL_Color from = ToSDL(b.Color), to = ToSDL(Rose);
                    b.To(ring, {Four(TweenProperty::StrokeColor, {from.r, from.g, from.b, 255.0f}, {to.r, to.g, to.b, 255.0f})}, 2.0f, 0.6f, "linear");
                    const entt::entity label = b.Label("3", {0, 0}, r * 0.7f, White, 0.1f);
                    b.Value(0.0f, 3.0f, "linear", [&reg = b.R, label](float v)
                            {
                                if (!reg.valid(label))
                                    return;
                                reg.get<TextComponent>(label).Text = std::to_string(std::max(0, static_cast<int>(std::ceil(3.0f - v * 3.0f))));
                            }); });
            Add(list, "ScorePop", "Game UI", "+100 pops up, rises and fades (text)", 1.1f, [](MotionBuilder &b)
                {
                    const entt::entity e = b.Label(b.TextOr("+100"), {0, 0}, b.SizeOr(46.0f), b.ColorOr(Gold), 0.1f);
                    b.Glow(e, b.ColorOr(Gold), 0.8f, 0.8f);
                    b.Pop(e, 0.0f, 0.3f);
                    b.Fly(e, {0, 0}, {0.0f, 70.0f}, 0.0f, 1.0f, "easeOutCubic");
                    b.To(e, {AlphaT(1.0f, 0.0f)}, 0.7f, 0.35f, "easeIn"); }, MotionInput::Text);
            Add(list, "DamageNumber", "Game UI", "A red damage number pops, shakes and falls away (text)", 1.1f, [](MotionBuilder &b)
                {
                    const entt::entity e = b.Label(b.TextOr("-25"), {0, 0}, b.SizeOr(50.0f), b.ColorOr(Rose), 0.1f);
                    b.Pop(e, 0.0f, 0.25f, "easeOutBack");
                    b.To(e, {One(TweenProperty::Rotation, -10.0f, 10.0f)}, 0.25f, 0.05f, "linear", 3, true);
                    b.Along(e, {{0, 0}, {30.0f, 40.0f}, {60.0f, -40.0f}}, 0.45f, 0.6f, "easeInQuad");
                    b.To(e, {AlphaT(1.0f, 0.0f)}, 0.75f, 0.3f, "easeIn"); }, MotionInput::Text);
            Add(list, "ComboText", "Game UI", "A combo callout stretches in and pulses (text)", 1.6f, [](MotionBuilder &b)
                {
                    const entt::entity e = b.Label(b.TextOr("x3 COMBO!"), {0, 0}, b.SizeOr(56.0f), b.ColorOr(Orange), 0.1f);
                    b.Glow(e, b.ColorOr(Orange), 1.2f, 1.0f);
                    b.Tf(e).rotation = -8.0f;
                    b.SetScale(e, 0.0f);
                    b.To(e, {Two(TweenProperty::Scale, {2.2f, 0.2f}, {1.0f, 1.0f})}, 0.0f, 0.45f, "easeOutBack");
                    b.To(e, {ScaleT(1.0f, 1.12f)}, 0.5f, 0.18f, "easeInOutSine", 3, true); }, MotionInput::Text);
            Add(list, "TapHint", "Game UI", "A finger-tap hint: a dot presses and ripples, twice", 2.2f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(26.0f);
                    for (int k = 0; k < 2; ++k)
                    {
                        const float t = 1.0f * static_cast<float>(k);
                        const entt::entity dot = b.Fill(VectorPath::Circle(r), {0, 0}, WithA(White, 0.85f), 0.1f);
                        b.SetAlpha(dot, 0.0f);
                        b.To(dot, {AlphaT(0.0f, 0.85f), ScaleT(1.3f, 0.9f)}, t, 0.25f, "easeOut");
                        b.To(dot, {AlphaT(0.85f, 0.0f)}, t + 0.45f, 0.25f, "easeIn");
                        const entt::entity ring = b.Stroke(VectorPath::Circle(r), {0, 0}, White, 3.0f, 0.0f);
                        b.SetAlpha(ring, 0.0f);
                        b.To(ring, {ScaleT(1.0f, 2.6f), AlphaT(0.9f, 0.0f)}, t + 0.2f, 0.6f, "easeOut");
                    } });
            Add(list, "SwipeHint", "Game UI", "A swipe hint: a dot slides with a fading trail, twice (points)", 2.4f, [](MotionBuilder &b)
                {
                    const std::vector<glm::vec2> pts = Local(b, {{-120.0f, 0.0f}, {120.0f, 0.0f}});
                    for (int k = 0; k < 2; ++k)
                    {
                        const float t = 1.1f * static_cast<float>(k);
                        const entt::entity trail = b.Line(pts, WithA(White, 0.5f), 18.0f);
                        b.SetTrim(trail, 0.0f, 0.0f);
                        b.To(trail, {One(TweenProperty::TrimEnd, 0.0f, 1.0f)}, t, 0.6f, "easeInOutCubic");
                        b.To(trail, {One(TweenProperty::TrimStart, 0.0f, 1.0f)}, t + 0.2f, 0.6f, "easeInOutCubic");
                        const entt::entity dot = b.Fill(VectorPath::Circle(18.0f), pts.front(), White, 0.1f);
                        b.SetAlpha(dot, 0.0f);
                        b.To(dot, {AlphaT(0.0f, 0.9f)}, t, 0.1f, "linear");
                        b.Along(dot, pts, t, 0.6f, "easeInOutCubic");
                        b.To(dot, {AlphaT(0.9f, 0.0f)}, t + 0.6f, 0.2f, "linear");
                    } }, MotionInput::Points);
            Add(list, "FocusBrackets", "Game UI", "Corner brackets close in around a target", 1.0f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(70.0f);
                    const float l = r * 0.4f;
                    for (int i = 0; i < 4; ++i)
                    {
                        const glm::vec2 corner{(i % 2 ? 1.0f : -1.0f) * r, (i < 2 ? 1.0f : -1.0f) * r};
                        const glm::vec2 hx{(i % 2 ? -1.0f : 1.0f) * l, 0.0f}, hy{0.0f, (i < 2 ? -1.0f : 1.0f) * l};
                        const entt::entity bracket = b.Node("Bracket", corner * 1.8f, 0.1f);
                        b.Line({hx, {0, 0}, hy}, b.ColorOr(Gold), 5.0f, 0.0f, false, bracket);
                            b.To(bracket, {MoveT(corner * 1.8f, corner)}, 0.0f, 0.55f, "easeOutBack");
                    } });
            Add(list, "ChatTyping", "Game UI", "A chat bubble with three typing dots", 2.0f, [](MotionBuilder &b)
                {
                    const float h = b.SizeOr(50.0f);
                    const entt::entity bubble = b.Node("Bubble", {0, 0});
                    b.Fill(VectorPath::Rect(h * 2.4f, h, h * 0.5f), {0, 0}, glm::vec4(0.2f, 0.24f, 0.34f, 1.0f), 0.0f, bubble);
                    b.Pop(bubble, 0.0f, 0.35f);
                    for (int i = 0; i < 3; ++i)
                    {
                        const glm::vec2 p{(static_cast<float>(i) - 1.0f) * h * 0.55f, 0.0f};
                        const entt::entity dot = b.Fill(VectorPath::Circle(h * 0.12f), p, White, 0.01f, bubble);
                        b.To(dot, {MoveT(p, p + glm::vec2(0.0f, h * 0.15f)), AlphaT(0.5f, 1.0f)}, 0.3f + 0.15f * static_cast<float>(i), 0.25f, "easeInOutSine", 5, true);
                    } });

            const auto ambient = [](MotionBuilder &b, int count, float area, const std::function<void(MotionBuilder &, int, glm::vec2)> &each)
            {
                for (int i = 0; i < count; ++i)
                    each(b, i, glm::vec2((Hash(i) - 0.5f) * area * 2.0f, (Hash(i + 101) - 0.5f) * area * 1.2f));
            };
            Add(list, "Rain", "Ambient", "Rain streaks fall", 2.0f, [ambient](MotionBuilder &b)
                {
                    const float a = b.SizeOr(200.0f);
                    ambient(b, 40, a, [a](MotionBuilder &m, int i, glm::vec2 p)
                            {
                                const entt::entity drop = m.Line({{0, 0}, {-4.0f, -22.0f}}, WithA(Sky, 0.7f), 2.0f);
                                const glm::vec2 top{p.x, a * 0.7f};
                                m.Tf(drop).position = top;
                                m.To(drop, {MoveT(top, {p.x - 30.0f, -a * 0.7f})}, Hash(i + 7) * 0.8f, 0.6f + 0.3f * Hash(i + 9), "linear", 2);
                            }); });
            Add(list, "Snow", "Ambient", "Snowflakes drift down and sway", 3.0f, [ambient](MotionBuilder &b)
                {
                    const float a = b.SizeOr(200.0f);
                    ambient(b, 30, a, [a](MotionBuilder &m, int i, glm::vec2 p)
                            {
                                const entt::entity flake = m.Fill(VectorPath::Circle(2.5f + 3.0f * Hash(i + 5)), {p.x, a * 0.7f}, WithA(White, 0.85f));
                                m.Along(flake, {{p.x, a * 0.7f}, {p.x + 25.0f, a * 0.2f}, {p.x - 25.0f, -a * 0.2f}, {p.x + 10.0f, -a * 0.7f}}, Hash(i + 3) * 1.0f, 2.0f, "linear");
                            }); });
            Add(list, "Bubbles", "Ambient", "Bubbles rise, wobble and pop", 2.4f, [ambient](MotionBuilder &b)
                {
                    const float a = b.SizeOr(200.0f);
                    ambient(b, 14, a, [a](MotionBuilder &m, int i, glm::vec2 p)
                            {
                                const float r = 6.0f + 12.0f * Hash(i + 2);
                                const entt::entity bubble = m.Stroke(VectorPath::Circle(r), {p.x, -a * 0.7f}, WithA(Sky, 0.8f), 2.0f);
                                const float t = Hash(i + 4) * 0.8f;
                                m.Along(bubble, {{p.x, -a * 0.7f}, {p.x + 15.0f, -a * 0.1f}, {p.x - 10.0f, a * 0.4f}}, t, 1.4f, "easeOutSine");
                                m.To(bubble, {ScaleT(1.0f, 1.5f), AlphaT(1.0f, 0.0f)}, t + 1.35f, 0.15f, "easeOut");
                            }); });
            Add(list, "Fireflies", "Ambient", "Glowing fireflies wander and blink", 3.0f, [ambient](MotionBuilder &b)
                {
                    const float a = b.SizeOr(200.0f);
                    ambient(b, 14, a * 0.8f, [](MotionBuilder &m, int i, glm::vec2 p)
                            {
                                const entt::entity fly = m.Fill(VectorPath::Circle(4.0f), p, glm::vec4(0.85f, 1.0f, 0.5f, 1.0f));
                                m.Glow(fly, glm::vec4(0.75f, 1.0f, 0.35f, 1.0f), 2.0f, 1.0f);
                                m.Along(fly, {p, p + glm::vec2(40.0f * (Hash(i) - 0.5f), 40.0f * Hash(i + 1)), p + glm::vec2(60.0f * (Hash(i + 2) - 0.5f), 20.0f)}, 0.0f, 2.8f, "easeInOutSine");
                                m.To(fly, {AlphaT(1.0f, 0.15f)}, Hash(i + 8) * 0.6f, 0.4f + 0.3f * Hash(i + 9), "easeInOutSine", 3, true);
                            }); });
            Add(list, "Starfield", "Ambient", "Stars twinkle across the sky", 2.6f, [ambient](MotionBuilder &b)
                {
                    const float a = b.SizeOr(220.0f);
                    ambient(b, 40, a, [](MotionBuilder &m, int i, glm::vec2 p)
                            {
                                const entt::entity star = m.Fill(VectorPath::Star(4, 3.0f + 4.0f * Hash(i + 1), 1.0f + 1.2f * Hash(i + 2)), p, White);
                                m.SetAlpha(star, 0.0f);
                                m.To(star, {AlphaT(0.1f, 1.0f)}, Hash(i + 3) * 0.8f, 0.4f + 0.4f * Hash(i + 4), "easeInOutSine", 3, true);
                            }); });
            Add(list, "MeteorShower", "Ambient", "Meteors streak diagonally across", 2.4f, [](MotionBuilder &b)
                {
                    const float a = b.SizeOr(220.0f);
                    for (int i = 0; i < 6; ++i)
                    {
                        const glm::vec2 from{-a * 1.2f + a * 0.5f * Hash(i), a * 0.4f + a * 0.4f * Hash(i + 1)};
                        const glm::vec2 to = from + glm::vec2(a * 1.6f, -a * 1.0f);
                        const entt::entity streak = b.Line({from, to}, WithA(White, 0.9f), 3.0f);
                        b.Glow(streak, Sky, 1.2f, 0.8f);
                        b.SetTrim(streak, 0.0f, 0.0f);
                        const float t = 0.3f * static_cast<float>(i);
                        b.To(streak, {One(TweenProperty::TrimEnd, 0.0f, 1.0f)}, t, 0.5f, "easeIn");
                        b.To(streak, {One(TweenProperty::TrimStart, 0.0f, 1.0f)}, t + 0.15f, 0.5f, "easeIn");
                    } });
            Add(list, "Explosion", "Effects", "A flash, a shockwave and flying debris", 1.3f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(80.0f);
                    const entt::entity flash = b.Fill(VectorPath::Circle(r * 0.8f), {0, 0}, White, 0.2f);
                    b.Glow(flash, Orange, 3.0f, 1.4f);
                    b.To(flash, {ScaleT(0.2f, 1.4f), AlphaT(1.0f, 0.0f)}, 0.0f, 0.35f, "easeOut");
                    const entt::entity wave = b.Stroke(VectorPath::Circle(r), {0, 0}, Orange, 12.0f, 0.1f);
                    b.To(wave, {ScaleT(0.3f, 2.4f), One(TweenProperty::StrokeWidth, 14.0f, 1.0f), AlphaT(1.0f, 0.0f)}, 0.05f, 0.7f, "easeOutCubic");
                    for (int i = 0; i < 14; ++i)
                    {
                        const glm::vec2 d = Dir(360.0f * Hash(i));
                        const entt::entity bit = b.Fill(VectorPath::Polygon(3, 5.0f + 6.0f * Hash(i + 3)), {0, 0}, PaletteColor(i % 3 == 0 ? 0 : 1, Gold, Orange), 0.15f);
                        b.Along(bit, {{0, 0}, d * r * (1.5f + Hash(i + 5)), d * r * (2.2f + Hash(i + 5)) + glm::vec2(0.0f, -r * 0.8f)}, 0.05f, 1.0f, "easeOutQuad");
                        b.To(bit, {One(TweenProperty::Rotation, 0.0f, 600.0f * (Hash(i + 9) - 0.5f))}, 0.05f, 1.0f, "linear");
                        b.To(bit, {AlphaT(1.0f, 0.0f)}, 0.8f, 0.3f, "easeIn");
                    } });
            Add(list, "Portal", "Effects", "Glowing rings spin open into a portal", 2.4f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(80.0f);
                    for (int i = 0; i < 4; ++i)
                    {
                        const float k = 1.0f - 0.2f * static_cast<float>(i);
                        const entt::entity ring = b.Line(Arc({0, 0}, r * k, 0.0f, 300.0f, 64), PaletteColor(i % 2 ? 3 : 2, b.Color, b.Accent), 5.0f, 0.01f * static_cast<float>(i));
                        b.Glow(ring, Violet, 1.5f, 1.0f);
                        b.Pop(ring, 0.1f * static_cast<float>(i), 0.5f);
                        b.To(ring, {One(TweenProperty::Rotation, 0.0f, (i % 2 ? -1.0f : 1.0f) * 540.0f)}, 0.0f, 2.4f, "linear");
                    }
                    const entt::entity core = b.Fill(VectorPath::Circle(r * 0.3f), {0, 0}, WithA(Violet, 0.6f), -0.01f);
                    b.To(core, {ScaleT(0.0f, 1.0f)}, 0.4f, 0.6f, "easeOutBack");
                    b.To(core, {ScaleT(1.0f, 1.2f)}, 1.0f, 0.4f, "easeInOutSine", 2, true); });
            Add(list, "Lightning", "Effects", "A lightning bolt strikes and flickers (points)", 1.0f, [](MotionBuilder &b)
                {
                    const std::vector<glm::vec2> ends = Local(b, {{-30.0f, 200.0f}, {20.0f, -160.0f}});
                    std::vector<glm::vec2> pts{ends.front()};
                    const glm::vec2 d = ends.back() - ends.front();
                    const glm::vec2 n = glm::normalize(glm::vec2(-d.y, d.x));
                    for (int i = 1; i < 9; ++i)
                        pts.push_back(ends.front() + d * (static_cast<float>(i) / 9.0f) + n * (Hash(i) - 0.5f) * 60.0f);
                    pts.push_back(ends.back());
                    const entt::entity bolt = b.Line(pts, White, b.WidthOr(5.0f));
                    b.Glow(bolt, Sky, 2.5f, 1.2f);
                    b.Draw(bolt, 0.0f, 0.12f, "linear");
                    b.To(bolt, {AlphaT(1.0f, 0.2f)}, 0.15f, 0.06f, "linear", 3, true);
                    b.To(bolt, {AlphaT(1.0f, 0.0f)}, 0.6f, 0.3f, "easeIn"); }, MotionInput::Points);
            Add(list, "MagicCircle", "Effects", "A rune circle draws itself and slowly turns", 2.6f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(100.0f);
                    const entt::entity spin = b.Node("Runes", {0, 0});
                    const glm::vec4 c = b.ColorOr(Violet);
                    b.Draw(b.Stroke(VectorPath::Circle(r), {0, 0}, c, 3.0f, 0.0f, spin), 0.0f, 0.6f);
                    b.Draw(b.Stroke(VectorPath::Circle(r * 0.82f), {0, 0}, c, 2.0f, 0.0f, spin), 0.15f, 0.6f);
                    b.Draw(b.Stroke(VectorPath::Star(6, r * 0.8f, r * 0.46f), {0, 0}, c, 2.5f, 0.0f, spin), 0.4f, 0.8f);
                    b.Draw(b.Stroke(VectorPath::Polygon(6, r * 0.46f), {0, 0}, c, 2.0f, 0.0f, spin), 0.8f, 0.5f);
                    for (int i = 0; i < 6; ++i)
                        b.Pop(b.Fill(VectorPath::Circle(6.0f), Dir(60.0f * static_cast<float>(i)) * r * 0.91f, b.Accent, 0.01f, spin), 1.0f + 0.07f * static_cast<float>(i), 0.3f);
                    b.To(spin, {One(TweenProperty::Rotation, 0.0f, 120.0f)}, 0.0f, 2.6f, "easeInOutSine"); });
            Add(list, "RadarSweep", "Effects", "A radar beam sweeps and blips light up", 2.4f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(100.0f);
                    for (int i = 1; i <= 3; ++i)
                        b.FadeIn(b.Stroke(VectorPath::Circle(r * static_cast<float>(i) / 3.0f), {0, 0}, WithA(Lime, 0.35f), 1.5f, -0.01f), 0.0f, 0.3f);
                    const entt::entity beam = b.Node("Beam", {0, 0}, 0.1f);
                    b.Fill(Pie(r, 0.0f, -35.0f), {0, 0}, WithA(Lime, 0.3f), 0.0f, beam);
                    b.Line({{0, 0}, {r, 0}}, Lime, 2.5f, 0.01f, false, beam);
                    b.To(beam, {One(TweenProperty::Rotation, 0.0f, -720.0f)}, 0.0f, 2.4f, "linear");
                    for (int i = 0; i < 4; ++i)
                    {
                        const float ang = -90.0f * static_cast<float>(i) - 50.0f;
                        const entt::entity blip = b.Fill(VectorPath::Circle(5.0f), Dir(ang) * r * (0.35f + 0.5f * Hash(i)), Lime, 0.05f);
                        b.Glow(blip, Lime, 1.5f, 0.8f);
                        b.SetScale(blip, 0.0f);
                        const float t = (-ang / 360.0f) * 1.2f;
                        b.To(blip, {ScaleT(1.6f, 0.0f)}, t, 0.9f, "easeIn");
                    } });
            Add(list, "Heartline", "Effects", "A heart monitor line beats across", 2.0f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(340.0f);
                    std::vector<glm::vec2> pts;
                    for (int k = 0; k < 2; ++k)
                    {
                        const float x0 = -w * 0.5f + w * 0.5f * static_cast<float>(k);
                        for (glm::vec2 p : std::vector<glm::vec2>{{0.0f, 0.0f}, {0.2f, 0.0f}, {0.25f, 0.1f}, {0.3f, 0.0f}, {0.36f, 0.0f}, {0.4f, -0.15f}, {0.45f, 0.75f}, {0.5f, -0.35f}, {0.55f, 0.0f}, {0.68f, 0.0f}, {0.75f, 0.15f}, {0.82f, 0.0f}, {1.0f, 0.0f}})
                            pts.push_back({x0 + p.x * w * 0.5f, p.y * w * 0.25f});
                    }
                    const entt::entity line = b.Line(pts, b.ColorOr(Lime), b.WidthOr(3.5f));
                    b.Glow(line, b.ColorOr(Lime), 1.4f, 1.0f);
                    b.SetTrim(line, 0.0f, 0.0f);
                    b.To(line, {One(TweenProperty::TrimEnd, 0.0f, 1.0f)}, 0.0f, 1.5f, "linear");
                    b.To(line, {One(TweenProperty::TrimStart, 0.0f, 1.0f)}, 0.5f, 1.5f, "linear"); });

            Add(list, "ScrambleText", "Text", "Random letters settle into the text, left to right", 1.4f, [](MotionBuilder &b)
                {
                    const std::string text = b.TextOr("DECODED");
                    const entt::entity label = b.Label(text, {0, 0}, b.SizeOr(52.0f), b.ColorOr(Lime));
                    b.Glow(label, b.ColorOr(Lime), 0.7f, 0.8f);
                    b.Value(0.0f, 1.2f, "linear", [&reg = b.R, label, text](float v)
                            {
                                if (!reg.valid(label))
                                    return;
                                static const char glyphs[] = "ABCDEFGHJKLMNPQRSTUVWXYZ0123456789#$%&@";
                                std::string out = text;
                                const size_t settled = static_cast<size_t>(v * static_cast<float>(text.size() + 1));
                                const int frame = static_cast<int>(v * 40.0f);
                                for (size_t i = settled; i < out.size(); ++i)
                                    if (out[i] != ' ')
                                        out[i] = glyphs[static_cast<size_t>(Hash(static_cast<int>(i) * 31 + frame) * 38.0f) % 38];
                                reg.get<TextComponent>(label).Text = out;
                            }); }, MotionInput::Text);
            Add(list, "WordsPop", "Text", "Words pop in one at a time (text)", 1.6f, [](MotionBuilder &b)
                {
                    const std::string text = b.TextOr("one word at a time");
                    const float size = b.SizeOr(44.0f);
                    auto font = Font::GetDefault();
                    const float total = font ? font->MeasureText(text, size).x : size * 0.55f * static_cast<float>(text.size());
                    size_t start = 0;
                    int index = 0;
                    while (start < text.size())
                    {
                        size_t end = text.find(' ', start);
                        if (end == std::string::npos)
                            end = text.size();
                        const std::string word = text.substr(start, end - start);
                        const float before = font ? font->MeasureText(text.substr(0, start), size).x : size * 0.55f * static_cast<float>(start);
                        const float width = font ? font->MeasureText(word, size).x : size * 0.55f * static_cast<float>(word.size());
                        if (!word.empty())
                        {
                            const entt::entity e = b.Label(word, {-total * 0.5f + before + width * 0.5f, 0.0f}, size, PaletteColor(index == 1 ? 1 : 0, b.ColorOr(White), b.Accent));
                            b.Pop(e, 0.22f * static_cast<float>(index), 0.4f);
                            ++index;
                        }
                        start = end + 1;
                    } }, MotionInput::Text);
            Add(list, "TextSlide", "Text", "Text slides in from the left behind a moving bar (text)", 1.2f, [](MotionBuilder &b)
                {
                    const std::string text = b.TextOr("NEW AREA");
                    const float size = b.SizeOr(50.0f);
                    auto font = Font::GetDefault();
                    const float w = font ? font->MeasureText(text, size).x : size * 0.55f * static_cast<float>(text.size());
                    const entt::entity bar = b.Fill(VectorPath::Rect(w + 40.0f, size * 1.3f, 6.0f), {0, 0}, b.Accent, 0.1f);
                    b.Tf(bar).scale.x = 0.0f;
                    b.To(bar, {One(TweenProperty::ScaleX, 0.0f, 1.0f)}, 0.0f, 0.35f, "easeOutCubic");
                    b.To(bar, {One(TweenProperty::ScaleX, 1.0f, 0.0f), MoveT({0, 0}, {(w + 40.0f) * 0.5f, 0.0f})}, 0.4f, 0.35f, "easeInCubic");
                    const entt::entity label = b.Label(text, {0, 0}, size, b.ColorOr(White), 0.0f);
                    b.SetAlpha(label, 0.0f);
                    b.To(label, {AlphaT(0.0f, 1.0f)}, 0.35f, 0.05f, "linear"); }, MotionInput::Text);
            Add(list, "NeonText", "Text", "Neon sign text flickers on and hums (text)", 2.0f, [](MotionBuilder &b)
                {
                    const glm::vec4 c = b.ColorOr(Rose);
                    const entt::entity label = b.Label(b.TextOr("OPEN"), {0, 0}, b.SizeOr(70.0f), c);
                    b.Glow(label, c, 0.0f, 1.2f);
                    b.SetAlpha(label, 0.15f);
                    b.To(label, {AlphaT(0.15f, 1.0f)}, 0.1f, 0.05f, "linear", 3, true);
                    b.To(label, {AlphaT(0.2f, 1.0f), One(TweenProperty::GlowIntensity, 0.0f, 2.2f)}, 0.5f, 0.2f, "easeOut");
                    b.To(label, {One(TweenProperty::GlowIntensity, 2.2f, 1.6f)}, 0.8f, 0.3f, "easeInOutSine", 3, true); }, MotionInput::Text);
            Add(list, "Caption", "Text", "An explainer caption bar: a small heading and a line of text (text)", 2.2f, [](MotionBuilder &b)
                {
                    const std::string text = b.TextOr("Cut the area into thin slices");
                    const float size = b.SizeOr(34.0f);
                    auto font = Font::GetDefault();
                    const float w = (font ? font->MeasureText(text, size).x : size * 0.55f * static_cast<float>(text.size())) + size * 3.0f;
                    const entt::entity bar = b.Node("Caption", {0.0f, -16.0f});
                    b.Fill(VectorPath::Rect(w, size * 2.9f, size * 0.7f), {0, 0}, glm::vec4(0.06f, 0.07f, 0.12f, 0.85f), 0.0f, bar);
                    b.Label("STEP 1", {0.0f, size * 0.62f}, size * 0.6f, b.Accent, 0.01f, 1, bar);
                    b.Label(text, {0.0f, -size * 0.38f}, size, White, 0.01f, 1, bar);
                    b.SetScale(bar, 0.0f);
                    b.To(bar, {ScaleT(0.9f, 1.0f), MoveT({0.0f, -16.0f}, {0.0f, 0.0f})}, 0.0f, 0.45f, "easeOutBack");
                    b.To(bar, {Two(TweenProperty::Scale, {1.0f, 1.0f}, {1.0f, 0.0f})}, 1.8f, 0.3f, "easeInCubic"); }, MotionInput::Text);
        }

        std::vector<MotionPreset> BuildAll()
        {
            std::vector<MotionPreset> list;

            static const struct
            {
                const char *Style;
                const char *Text;
                float Duration;
            } styles[] = {
                {"Pop", "pops in with a little overshoot", 0.5f},
                {"GlowPop", "pops in with a glow flash that settles", 1.0f},
                {"Grow", "grows and fades in smoothly", 0.7f},
                {"Fade", "fades in while rising a little", 0.6f},
                {"Spin", "spins in from nothing", 0.8f},
                {"Drop", "drops from above and bounces", 0.9f},
                {"Elastic", "springs in with an elastic wobble", 1.1f},
                {"Draw", "outline draws itself, then the fill fades in", 1.15f},
                {"Ripple", "pops in and sends two ripples out", 1.3f},
            };
            for (const ShapeKind &kind : ShapeKinds())
                for (const auto &style : styles)
                {
                    const ShapeKind shape = kind;
                    const std::string styleName = style.Style;
                    Add(list, std::string(kind.Name) + style.Style, "Shapes", std::string("A ") + kind.Name + " that " + style.Text, style.Duration,
                        [shape, styleName](MotionBuilder &b)
                        { ShapeEntrance(b, shape, styleName); });
                }

            Add(list, "RippleRings", "Effects", "Three rings spread out from a point", 1.5f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(40.0f);
                    for (int i = 0; i < 3; ++i)
                    {
                        const entt::entity ring = b.Stroke(VectorPath::Circle(r), {0, 0}, b.Color, b.WidthOr(4.0f));
                        b.SetAlpha(ring, 0.0f);
                        b.SetScale(ring, 0.2f);
                        b.To(ring, {ScaleT(0.2f, 3.0f), AlphaT(1.0f, 0.0f)}, 0.25f * static_cast<float>(i), 1.0f, "easeOutCubic");
                    } });
            Add(list, "PulseDot", "Effects", "A dot with a ring that pulses three times", 2.2f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(18.0f);
                    const entt::entity dot = b.Fill(VectorPath::Circle(r), {0, 0}, b.Color, 0.01f);
                    b.Pop(dot, 0.0f, 0.35f);
                    const entt::entity ring = b.Stroke(VectorPath::Circle(r), {0, 0}, b.Color, 3.0f);
                    b.SetAlpha(ring, 0.0f);
                    b.To(ring, {ScaleT(1.0f, 3.2f), AlphaT(0.9f, 0.0f)}, 0.3f, 0.6f, "easeOut", 2); });
            Add(list, "Burst", "Effects", "Dots shoot out from the center and fade", 0.9f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(110.0f);
                    for (int i = 0; i < 12; ++i)
                    {
                        const glm::vec2 d = Dir(30.0f * static_cast<float>(i));
                        const entt::entity dot = b.Fill(VectorPath::Circle(r * 0.07f), {0, 0}, PaletteColor(i % 3, b.Color, b.Accent));
                        b.To(dot, {MoveT({0, 0}, d * r), ScaleT(1.0f, 0.2f)}, 0.0f, 0.8f, "easeOutCubic");
                        b.To(dot, {AlphaT(1.0f, 0.0f)}, 0.45f, 0.4f, "easeIn");
                    } });
            Add(list, "Sparkle", "Effects", "Little stars twinkle around a point", 1.4f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(90.0f);
                    static const glm::vec3 spots[] = {{-0.8f, 0.4f, 0.8f}, {0.6f, 0.7f, 1.0f}, {0.9f, -0.3f, 0.7f}, {-0.3f, -0.8f, 0.9f}, {0.1f, 0.1f, 1.2f}, {-0.9f, -0.2f, 0.6f}};
                    int i = 0;
                    for (const glm::vec3 &s : spots)
                    {
                        const entt::entity star = b.Fill(VectorPath::Star(4, r * 0.22f * s.z, r * 0.06f * s.z), glm::vec2(s.x, s.y) * r, i % 2 ? b.Accent : White);
                        b.Glow(star, b.Accent, 1.2f, 0.8f);
                        const float t = 0.12f * static_cast<float>(i++);
                        b.SetScale(star, 0.0f);
                        b.To(star, {ScaleT(0.0f, 1.0f), One(TweenProperty::Rotation, 0.0f, 90.0f)}, t, 0.4f, "easeOutBack");
                        b.To(star, {ScaleT(1.0f, 0.0f)}, t + 0.55f, 0.35f, "easeIn");
                    } });
            Add(list, "Confetti", "Effects", "Colored pieces burst up and fall", 1.8f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(160.0f);
                    for (int i = 0; i < 22; ++i)
                    {
                        const float a = 60.0f + 60.0f * static_cast<float>((i * 37) % 22) / 21.0f;
                        const float spread = 0.5f + static_cast<float>((i * 13) % 7) / 7.0f;
                        const glm::vec2 top = Dir(a) * r * spread;
                        const glm::vec2 end = glm::vec2(top.x * 1.4f, -r * (0.6f + 0.05f * static_cast<float>(i % 5)));
                        const entt::entity piece = b.Fill(VectorPath::Rect(10.0f, 16.0f, 2.0f), {0, 0}, PaletteColor(i % 7, b.Color, b.Accent));
                        b.Along(piece, {{0, 0}, top, end}, 0.0f, 1.6f, "easeOutQuad");
                        b.To(piece, {One(TweenProperty::Rotation, 0.0f, 540.0f + 40.0f * static_cast<float>(i))}, 0.0f, 1.6f, "linear");
                        b.To(piece, {AlphaT(1.0f, 0.0f)}, 1.2f, 0.4f, "easeIn");
                    } });
            Add(list, "Firework", "Effects", "A spark rises and bursts into a ring of light", 1.9f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(110.0f);
                    const glm::vec2 top{0.0f, r * 0.8f};
                    const entt::entity trail = b.Line({{0.0f, -r * 1.4f}, top}, b.Accent, 4.0f);
                    b.To(trail, {One(TweenProperty::TrimEnd, 0.0f, 1.0f), One(TweenProperty::TrimStart, 0.0f, 0.85f)}, 0.0f, 0.6f, "easeOutCubic");
                    b.SetTrim(trail, 0.0f, 0.0f);
                    b.To(trail, {AlphaT(1.0f, 0.0f)}, 0.55f, 0.2f, "easeIn");
                    for (int i = 0; i < 16; ++i)
                    {
                        const glm::vec2 d = Dir(22.5f * static_cast<float>(i));
                        const entt::entity spark = b.Fill(VectorPath::Circle(5.0f), top, PaletteColor(i % 4, b.Color, b.Accent));
                        b.Glow(spark, b.Accent, 1.4f, 0.9f);
                        b.SetScale(spark, 0.0f);
                        b.To(spark, {ScaleT(0.0f, 1.0f)}, 0.6f, 0.1f, "easeOut");
                        b.To(spark, {MoveT(top, top + d * r)}, 0.6f, 0.9f, "easeOutCubic");
                        b.To(spark, {AlphaT(1.0f, 0.0f)}, 1.1f, 0.6f, "easeIn");
                    } });
            Add(list, "Shockwave", "Effects", "A thick glowing ring blasts outward and thins", 0.9f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(40.0f);
                    const entt::entity ring = b.Stroke(VectorPath::Circle(r), {0, 0}, b.Color, b.WidthOr(16.0f));
                    b.Glow(ring, b.Color, 2.0f, 1.2f);
                    b.To(ring, {ScaleT(0.3f, 3.5f), One(TweenProperty::StrokeWidth, b.WidthOr(16.0f), 1.0f)}, 0.0f, 0.85f, "easeOutCubic");
                    b.To(ring, {AlphaT(1.0f, 0.0f)}, 0.35f, 0.5f, "easeIn"); });
            Add(list, "Heartbeat", "Effects", "A heart beats twice", 1.6f, [](MotionBuilder &b)
                {
                    const entt::entity heart = b.Fill(VectorPath::Heart(b.SizeOr(60.0f) * 2.0f), {0, 0}, b.ColorOr(Rose));
                    b.Pop(heart, 0.0f, 0.4f);
                    b.To(heart, {ScaleT(1.0f, 1.25f)}, 0.5f, 0.14f, "easeOut", 3, true); });
            Add(list, "Orbit", "Effects", "Three moons circle a planet", 2.0f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(90.0f);
                    b.Pop(b.Fill(VectorPath::Circle(r * 0.3f), {0, 0}, b.Color, 0.01f), 0.0f, 0.4f);
                    const entt::entity pivot = b.Node("Pivot", {0, 0});
                    b.Pop(b.Stroke(VectorPath::Circle(r), {0, 0}, WithA(White, 0.2f), 1.5f, -0.01f), 0.0f, 0.5f);
                    for (int i = 0; i < 3; ++i)
                        b.Pop(b.Fill(VectorPath::Circle(r * 0.1f), Dir(120.0f * static_cast<float>(i)) * r, PaletteColor(i + 1, b.Color, b.Accent), 0.02f, pivot), 0.2f + 0.1f * static_cast<float>(i), 0.35f);
                    b.To(pivot, {One(TweenProperty::Rotation, 0.0f, 360.0f)}, 0.0f, 2.0f, "linear"); });
            Add(list, "Spinner", "Effects", "A loading arc that spins", 1.6f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(40.0f);
                    b.Pop(b.Stroke(VectorPath::Circle(r), {0, 0}, WithA(White, 0.15f), b.WidthOr(7.0f), -0.01f), 0.0f, 0.3f);
                    const entt::entity arc = b.Line(Arc({0, 0}, r, 90.0f, -180.0f), b.Color, b.WidthOr(7.0f));
                    b.To(arc, {One(TweenProperty::Rotation, 0.0f, -720.0f)}, 0.0f, 1.6f, "easeInOut"); });
            Add(list, "DotsLoader", "Effects", "Three dots bounce one after another", 1.8f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(12.0f);
                    for (int i = 0; i < 3; ++i)
                    {
                        const glm::vec2 p{(static_cast<float>(i) - 1.0f) * r * 3.0f, 0.0f};
                        const entt::entity dot = b.Fill(VectorPath::Circle(r), p, b.Color);
                        b.To(dot, {MoveT(p, p + glm::vec2(0.0f, r * 2.0f))}, 0.15f * static_cast<float>(i), 0.3f, "easeOutQuad", 3, true);
                    } });
            Add(list, "WaveDots", "Effects", "A row of dots rolls like a wave", 2.0f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(10.0f);
                    for (int i = 0; i < 7; ++i)
                    {
                        const glm::vec2 p{(static_cast<float>(i) - 3.0f) * r * 3.0f, 0.0f};
                        const entt::entity dot = b.Fill(VectorPath::Circle(r), p, PaletteColor(i % 2, b.Color, b.Accent));
                        b.To(dot, {MoveT(p, p + glm::vec2(0.0f, r * 2.5f)), ScaleT(1.0f, 1.3f)}, 0.08f * static_cast<float>(i), 0.4f, "easeInOutSine", 3, true);
                    } });
            Add(list, "Pendulum", "Effects", "A pendulum swings back and forth", 2.4f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(120.0f);
                    const entt::entity pivot = b.Node("Pivot", {0.0f, r * 0.5f});
                    b.Tf(pivot).rotation = -35.0f;
                    b.Line({{0, 0}, {0, -r}}, WithA(White, 0.7f), 3.0f, 0.0f, false, pivot);
                    b.Fill(VectorPath::Circle(r * 0.16f), {0.0f, -r}, b.Color, 0.01f, pivot);
                    b.Fill(VectorPath::Circle(5.0f), {0, 0}, White, 0.02f, pivot);
                    b.To(pivot, {One(TweenProperty::Rotation, -35.0f, 35.0f)}, 0.0f, 0.6f, "easeInOutSine", 3, true); });
            Add(list, "Gauge", "Effects", "A gauge needle sweeps to a value (values[1] is 0..1)", 1.4f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(90.0f);
                    const float value = std::clamp(b.ValuesOr({0.72f})[0], 0.0f, 1.0f);
                    const entt::entity track = b.Line(Arc({0, 0}, r, 180.0f, 0.0f), WithA(White, 0.15f), b.WidthOr(12.0f), -0.01f);
                    b.FadeIn(track, 0.0f, 0.3f);
                    const entt::entity fill = b.Line(Arc({0, 0}, r, 180.0f, 0.0f), b.Color, b.WidthOr(12.0f));
                    b.SetTrim(fill, 0.0f, 0.0f);
                    b.To(fill, {One(TweenProperty::TrimEnd, 0.0f, value)}, 0.2f, 1.0f, "easeOutCubic");
                    const entt::entity needle = b.Node("Needle", {0, 0}, 0.02f);
                    b.Fill(VectorPath::Points({{0.0f, -4.0f}, {r * 0.85f, 0.0f}, {0.0f, 4.0f}}, true), {0, 0}, White, 0.0f, needle);
                    b.Fill(VectorPath::Circle(8.0f), {0, 0}, White, 0.01f, needle);
                    b.Tf(needle).rotation = 180.0f;
                    b.To(needle, {One(TweenProperty::Rotation, 180.0f, 180.0f - 180.0f * value)}, 0.2f, 1.0f, "easeOutBack"); });

            Add(list, "ProgressBar", "UI", "A bar fills from left to right (values[1] is 0..1)", 1.2f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(260.0f);
                    const float h = b.WidthOr(16.0f);
                    const float value = std::clamp(b.ValuesOr({1.0f})[0], 0.0f, 1.0f);
                    b.FadeIn(b.Fill(VectorPath::Rect(w, h, h * 0.5f), {0, 0}, WithA(White, 0.12f), -0.01f), 0.0f, 0.3f);
                    const entt::entity fill = b.Line({{-w * 0.5f + h * 0.5f, 0.0f}, {w * 0.5f - h * 0.5f, 0.0f}}, b.Color, h);
                    b.SetTrim(fill, 0.0f, 0.0f);
                    b.To(fill, {One(TweenProperty::TrimEnd, 0.0f, value)}, 0.15f, 1.0f, "easeInOutCubic"); });
            Add(list, "ProgressRing", "UI", "A ring fills around like a timer (values[1] is 0..1)", 1.3f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(60.0f);
                    const float value = std::clamp(b.ValuesOr({1.0f})[0], 0.0f, 1.0f);
                    b.FadeIn(b.Stroke(VectorPath::Circle(r), {0, 0}, WithA(White, 0.12f), b.WidthOr(10.0f), -0.01f), 0.0f, 0.3f);
                    const entt::entity ring = b.Line(Arc({0, 0}, r, 90.0f, -270.0f, 96), b.Color, b.WidthOr(10.0f));
                    b.SetTrim(ring, 0.0f, 0.0f);
                    b.To(ring, {One(TweenProperty::TrimEnd, 0.0f, value)}, 0.15f, 1.1f, "easeInOutCubic");
                    const entt::entity label = b.Label("0%", {0, 0}, r * 0.5f, White, 0.02f);
                    b.Value(0.15f, 1.1f, "easeInOutCubic", [&reg = b.R, label, value](float v)
                            {
                                if (!reg.valid(label))
                                    return;
                                char t[16];
                                std::snprintf(t, sizeof(t), "%d%%", static_cast<int>(std::round(v * value * 100.0f)));
                                reg.get<TextComponent>(label).Text = t;
                            }); });
            Add(list, "Checkmark", "UI", "A green circle pops and a check draws inside", 1.0f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(50.0f);
                    const glm::vec4 c = b.ColorOr(Lime);
                    const entt::entity circle = b.Fill(VectorPath::Circle(r), {0, 0}, c);
                    b.Glow(circle, c, 0.9f, 1.0f);
                    b.Pop(circle, 0.0f, 0.45f);
                    const entt::entity check = b.Line({{-r * 0.45f, 0.0f}, {-r * 0.12f, -r * 0.32f}, {r * 0.48f, r * 0.32f}}, White, b.WidthOr(r * 0.16f), 0.01f);
                    b.Draw(check, 0.35f, 0.45f, "easeOutCubic"); });
            Add(list, "CrossMark", "UI", "A red circle pops and an X draws inside", 1.0f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(50.0f);
                    const glm::vec4 c = b.ColorOr(Rose);
                    b.Pop(b.Fill(VectorPath::Circle(r), {0, 0}, c), 0.0f, 0.45f);
                    const float k = r * 0.36f;
                    b.Draw(b.Line({{-k, k}, {k, -k}}, White, b.WidthOr(r * 0.16f), 0.01f), 0.35f, 0.25f);
                    b.Draw(b.Line({{k, k}, {-k, -k}}, White, b.WidthOr(r * 0.16f), 0.01f), 0.55f, 0.25f); });
            Add(list, "Toggle", "UI", "A switch slides on and lights up", 0.9f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(90.0f);
                    const float h = w * 0.52f;
                    const entt::entity track = b.Fill(VectorPath::Rect(w, h, h * 0.5f), {0, 0}, WithA(White, 0.2f), 0.0f);
                    b.FadeIn(track, 0.0f, 0.2f);
                    const glm::vec4 on = b.Color;
                    const entt::entity lit = b.Fill(VectorPath::Rect(w, h, h * 0.5f), {0, 0}, on, 0.01f);
                    b.SetAlpha(lit, 0.0f);
                    b.To(lit, {AlphaT(0.0f, 1.0f)}, 0.3f, 0.35f, "easeOut");
                    const entt::entity knob = b.Fill(VectorPath::Circle(h * 0.4f), {-w * 0.5f + h * 0.5f, 0.0f}, White, 0.02f);
                    b.Fly(knob, {-w * 0.5f + h * 0.5f, 0.0f}, {w * 0.5f - h * 0.5f, 0.0f}, 0.3f, 0.45f, "easeOutBack"); });
            Add(list, "Badge", "UI", "A circle badge pops with a number (text)", 0.9f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(34.0f);
                    const entt::entity badge = b.Fill(VectorPath::Circle(r), {0, 0}, b.ColorOr(Rose));
                    b.Pop(badge, 0.0f, 0.5f, "elasticOut");
                    b.Pop(b.Label(b.TextOr("3"), {0, 0}, r * 1.05f, White, 0.01f), 0.2f, 0.4f); }, MotionInput::Text);
            Add(list, "Pin", "UI", "A map pin drops in with a shadow", 1.0f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(30.0f);
                    const entt::entity shadow = b.Fill(VectorPath::Ellipse(r * 0.8f, r * 0.25f), {0, 0}, WithA(glm::vec4(0, 0, 0, 1), 0.35f), -0.01f);
                    b.Pop(shadow, 0.3f, 0.4f);
                    const entt::entity pin = b.Node("Pin", {0, 0}, 0.01f);
                    b.Fill(VectorPath::Points({{-r * 0.62f, r * 1.65f}, {0.0f, 0.0f}, {r * 0.62f, r * 1.65f}}, true), {0, 0}, b.ColorOr(Rose), 0.0f, pin);
                    b.Fill(VectorPath::Circle(r * 0.8f), {0.0f, r * 1.9f}, b.ColorOr(Rose), 0.0f, pin);
                    b.Fill(VectorPath::Circle(r * 0.32f), {0.0f, r * 1.9f}, White, 0.01f, pin);
                    b.FadeIn(pin, 0.0f, 0.1f);
                    b.Fly(pin, {0.0f, r * 6.0f}, {0.0f, 0.0f}, 0.0f, 0.75f, "bounceOut"); });
            Add(list, "Underline", "UI", "A line draws under a point (size is the length)", 0.6f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(220.0f);
                    b.Draw(b.Line({{-w * 0.5f, 0.0f}, {w * 0.5f, 0.0f}}, b.Color, b.WidthOr(6.0f)), 0.0f, 0.55f, "easeOutCubic"); });
            Add(list, "Strike", "UI", "A line strikes through, left to right", 0.5f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(220.0f);
                    b.Draw(b.Line({{-w * 0.5f, -4.0f}, {w * 0.5f, 6.0f}}, b.ColorOr(Rose), b.WidthOr(6.0f)), 0.0f, 0.45f, "easeOutCubic"); });
            Add(list, "CircleHighlight", "UI", "A hand drawn loop circles a spot", 0.9f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(80.0f);
                    std::vector<glm::vec2> pts;
                    for (int i = 0; i <= 80; ++i)
                    {
                        const float t = static_cast<float>(i) / 80.0f;
                        const float a = 200.0f - 400.0f * t;
                        const float k = 1.0f + 0.08f * t;
                        pts.push_back(Dir(a) * glm::vec2(r * 1.35f * k, r * 0.8f * k));
                    }
                    const entt::entity loop = b.Line(pts, b.ColorOr(Rose), b.WidthOr(5.0f));
                    b.Tf(loop).rotation = -6.0f;
                    b.Draw(loop, 0.0f, 0.85f, "easeInOutCubic"); });
            Add(list, "BoxHighlight", "UI", "A rounded box draws around a spot", 0.9f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(80.0f);
                    const entt::entity box = b.Stroke(VectorPath::Rect(r * 2.6f, r * 1.4f, 14.0f), {0, 0}, b.ColorOr(Gold), b.WidthOr(4.0f));
                    b.Draw(box, 0.0f, 0.85f, "easeInOutCubic"); });
            Add(list, "Spotlight", "UI", "A soft light ring closes in on a spot", 1.0f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(60.0f);
                    const entt::entity ring = b.Stroke(VectorPath::Circle(r), {0, 0}, b.ColorOr(Gold), b.WidthOr(5.0f));
                    b.Glow(ring, b.ColorOr(Gold), 1.6f, 1.2f);
                    b.SetAlpha(ring, 0.0f);
                    b.To(ring, {ScaleT(3.0f, 1.0f), AlphaT(0.0f, 1.0f)}, 0.0f, 0.8f, "easeOutCubic"); });

            Add(list, "CountUp", "Text", "A number counts up to values[1]", 1.4f, [](MotionBuilder &b)
                {
                    const float target = b.ValuesOr({100.0f})[0];
                    const entt::entity label = b.Label("0", {0, 0}, b.SizeOr(64.0f), b.ColorOr(White));
                    b.Pop(label, 0.0f, 0.35f);
                    const bool whole = std::abs(target - std::round(target)) < 1e-4f;
                    b.Value(0.1f, 1.2f, "easeOutCubic", [&reg = b.R, label, target, whole](float v)
                            {
                                if (!reg.valid(label))
                                    return;
                                char t[32];
                                if (whole)
                                    std::snprintf(t, sizeof(t), "%d", static_cast<int>(std::round(v * target)));
                                else
                                    std::snprintf(t, sizeof(t), "%.1f", v * target);
                                reg.get<TextComponent>(label).Text = t;
                            }); }, MotionInput::Values);
            Add(list, "Typewriter", "Text", "Text types itself out with a blinking caret", 1.6f, [](MotionBuilder &b)
                {
                    const std::string text = b.TextOr("Hello, world");
                    const float size = b.SizeOr(44.0f);
                    float width = size * 0.55f * static_cast<float>(text.size());
                    if (auto font = Font::GetDefault())
                        width = font->MeasureText(text, size).x;
                    const entt::entity label = b.Label("", {-width * 0.5f, 0.0f}, size, b.ColorOr(White), 0.0f, 0);
                    const entt::entity caret = b.Fill(VectorPath::Rect(3.0f, size * 0.9f), {-width * 0.5f, 0.0f}, b.Accent, 0.01f);
                    const size_t n = text.size();
                    b.Value(0.0f, 1.2f, "linear", [&reg = b.R, label, caret, text, n, width, size](float v)
                            {
                                if (!reg.valid(label))
                                    return;
                                const size_t count = std::min(n, static_cast<size_t>(v * static_cast<float>(n) + 0.5f));
                                const std::string shown = text.substr(0, count);
                                reg.get<TextComponent>(label).Text = shown;
                                float x = 0.0f;
                                if (auto font = Font::GetDefault())
                                    x = font->MeasureText(shown, size).x;
                                if (reg.valid(caret))
                                    reg.get<TransformComponent>(caret).position.x = -width * 0.5f + x + 4.0f;
                            });
                    b.To(caret, {AlphaT(1.0f, 0.0f)}, 1.2f, 0.12f, "linear", 3, true); }, MotionInput::Text);
            Add(list, "TitlePop", "Text", "A title pops in and an underline draws", 1.0f, [](MotionBuilder &b)
                {
                    const std::string text = b.TextOr("Level Up");
                    const float size = b.SizeOr(56.0f);
                    float width = size * 0.55f * static_cast<float>(text.size());
                    if (auto font = Font::GetDefault())
                        width = font->MeasureText(text, size).x;
                    b.Pop(b.Label(text, {0, 0}, size, b.ColorOr(White)), 0.0f, 0.5f);
                    b.Draw(b.Line({{-width * 0.5f, -size * 0.7f}, {width * 0.5f, -size * 0.7f}}, b.Accent, 6.0f), 0.35f, 0.5f, "easeOutCubic"); }, MotionInput::Text);
            Add(list, "GlowTitle", "Text", "A glowing title fades up", 1.2f, [](MotionBuilder &b)
                {
                    const entt::entity label = b.Label(b.TextOr("Victory"), {0, 0}, b.SizeOr(60.0f), b.ColorOr(Gold));
                    b.Glow(label, b.ColorOr(Gold), 0.0f, 1.0f);
                    b.FadeIn(label, 0.0f, 0.5f);
                    b.Fly(label, {0.0f, -30.0f}, {0.0f, 0.0f}, 0.0f, 0.6f);
                    b.To(label, {One(TweenProperty::GlowIntensity, 0.0f, 2.2f)}, 0.2f, 0.5f, "easeOut");
                    b.To(label, {One(TweenProperty::GlowIntensity, 2.2f, 1.0f)}, 0.7f, 0.5f, "easeInOut"); }, MotionInput::Text);
            Add(list, "TextRise", "Text", "Text fades in while rising", 0.7f, [](MotionBuilder &b)
                {
                    const entt::entity label = b.Label(b.TextOr("Rise up"), {0, 0}, b.SizeOr(48.0f), b.ColorOr(White));
                    b.FadeIn(label, 0.0f, 0.6f);
                    b.Fly(label, {0.0f, -40.0f}, {0.0f, 0.0f}, 0.0f, 0.65f); }, MotionInput::Text);

            const auto letters = [](MotionBuilder &b, const char *style)
            {
                const std::string text = b.TextOr("Motion");
                const float size = b.SizeOr(56.0f);
                auto font = Font::GetDefault();
                const float total = font ? font->MeasureText(text, size).x : size * 0.55f * static_cast<float>(text.size());
                float x = -total * 0.5f;
                const std::string mode = style;
                for (size_t i = 0; i < text.size(); ++i)
                {
                    const std::string ch = text.substr(i, 1);
                    const float w = font ? font->MeasureText(ch, size).x : size * 0.55f;
                    const glm::vec2 p{x + w * 0.5f, 0.0f};
                    x += font ? font->MeasureText(text.substr(0, i + 1), size).x - font->MeasureText(text.substr(0, i), size).x : w;
                    if (ch == " ")
                        continue;
                    const float t = 0.06f * static_cast<float>(i);
                    const entt::entity e = b.Label(ch, p, size, PaletteColor(mode == "Rainbow" ? i : 0, b.ColorOr(White), b.Accent));
                    if (mode == "Pop" || mode == "Rainbow")
                        b.Pop(e, t, 0.4f);
                    else if (mode == "Drop")
                    {
                        b.FadeIn(e, t, 0.2f);
                        b.Fly(e, p + glm::vec2(0.0f, size * 1.5f), p, t, 0.6f, "bounceOut");
                    }
                    else if (mode == "Wave")
                    {
                        b.FadeIn(e, t, 0.2f);
                        b.To(e, {MoveT(p, p + glm::vec2(0.0f, size * 0.3f))}, t, 0.35f, "easeInOutSine", 3, true);
                    }
                    else if (mode == "Spin")
                    {
                        b.SetScale(e, 0.0f);
                        b.Tf(e).rotation = -180.0f;
                        b.To(e, {ScaleT(0.0f, 1.0f), One(TweenProperty::Rotation, -180.0f, 0.0f)}, t, 0.5f, "easeOutBack");
                    }
                }
            };
            Add(list, "LettersPop", "Text", "Each letter pops in, one after another", 1.0f, [letters](MotionBuilder &b) { letters(b, "Pop"); }, MotionInput::Text);
            Add(list, "LettersDrop", "Text", "Letters fall in and bounce", 1.2f, [letters](MotionBuilder &b) { letters(b, "Drop"); }, MotionInput::Text);
            Add(list, "LettersWave", "Text", "Letters ride a wave", 2.0f, [letters](MotionBuilder &b) { letters(b, "Wave"); }, MotionInput::Text);
            Add(list, "LettersSpin", "Text", "Letters spin in one by one", 1.0f, [letters](MotionBuilder &b) { letters(b, "Spin"); }, MotionInput::Text);
            Add(list, "RainbowText", "Text", "Letters pop in, each in its own color", 1.0f, [letters](MotionBuilder &b) { letters(b, "Rainbow"); }, MotionInput::Text);

            Add(list, "LineDraw", "Lines", "A line draws through the points", 0.9f, [](MotionBuilder &b)
                { b.Draw(b.Line(Local(b, DefaultPath()), b.Color, b.WidthOr(6.0f)), 0.0f, 0.9f); }, MotionInput::Points);
            Add(list, "GlowLine", "Lines", "A glowing line draws through the points", 0.9f, [](MotionBuilder &b)
                {
                    const entt::entity line = b.Line(Local(b, DefaultPath()), b.Color, b.WidthOr(6.0f));
                    b.Glow(line, b.Color, 1.6f, 1.0f);
                    b.Draw(line, 0.0f, 0.9f); }, MotionInput::Points);
            Add(list, "DashedLine", "Lines", "A dashed line draws, then marches", 1.8f, [](MotionBuilder &b)
                {
                    const entt::entity line = b.Line(Local(b, DefaultPath()), b.Color, b.WidthOr(5.0f));
                    b.V(line).dashLength = 18.0f;
                    b.V(line).dashGap = 12.0f;
                    b.Draw(line, 0.0f, 0.8f);
                    b.To(line, {One(TweenProperty::DashOffset, 0.0f, -60.0f)}, 0.8f, 1.0f, "linear"); }, MotionInput::Points);
            Add(list, "PathTrace", "Lines", "A dot travels the path and leaves a trail", 1.3f, [](MotionBuilder &b)
                {
                    const std::vector<glm::vec2> pts = Local(b, DefaultPath());
                    b.FadeIn(b.Line(pts, WithA(White, 0.15f), b.WidthOr(5.0f), -0.01f), 0.0f, 0.2f);
                    b.Draw(b.Line(pts, b.Color, b.WidthOr(5.0f)), 0.1f, 1.1f, "easeInOut");
                    const entt::entity dot = b.Fill(VectorPath::Circle(b.WidthOr(5.0f) * 1.8f), pts.front(), b.Accent, 0.02f);
                    b.Glow(dot, b.Accent, 1.8f, 1.0f);
                    b.Along(dot, pts, 0.1f, 1.1f, "easeInOut"); }, MotionInput::Points);
            Add(list, "Comet", "Lines", "A glowing head races along with a short tail", 1.1f, [](MotionBuilder &b)
                {
                    const std::vector<glm::vec2> pts = Local(b, DefaultPath());
                    const entt::entity tail = b.Line(pts, b.Color, b.WidthOr(6.0f));
                    b.Glow(tail, b.Color, 1.6f, 1.0f);
                    b.SetTrim(tail, 0.0f, 0.0f);
                    b.To(tail, {One(TweenProperty::TrimEnd, 0.0f, 1.0f)}, 0.0f, 0.9f, "easeInOut");
                    b.To(tail, {One(TweenProperty::TrimStart, 0.0f, 1.0f)}, 0.15f, 0.9f, "easeInOut");
                    const entt::entity head = b.Fill(VectorPath::Circle(b.WidthOr(6.0f) * 1.5f), pts.front(), White, 0.02f);
                    b.Glow(head, b.Color, 2.0f, 1.0f);
                    b.Along(head, pts, 0.0f, 0.9f, "easeInOut");
                    b.To(head, {AlphaT(1.0f, 0.0f)}, 0.9f, 0.2f, "easeIn"); }, MotionInput::Points);
            Add(list, "Arrow", "Lines", "An arrow draws itself along the points", 0.8f, [](MotionBuilder &b)
                { BuildArrow(b, Local(b, DefaultPath()), false, false, false); }, MotionInput::Points);
            Add(list, "GlowArrow", "Lines", "A glowing arrow draws itself along the points", 0.8f, [](MotionBuilder &b)
                { BuildArrow(b, Local(b, DefaultPath()), false, false, true); }, MotionInput::Points);
            Add(list, "ArrowSlide", "Lines", "Arrow Out style: the arrow draws, then slides out along its path (size is the slide distance)", 2.1f, [](MotionBuilder &b)
                { BuildArrow(b, Local(b, {{-160.0f, -80.0f}, {-160.0f, 40.0f}, {0.0f, 40.0f}, {0.0f, -40.0f}, {110.0f, -40.0f}}), true, false, false); }, MotionInput::Points);
            Add(list, "DoubleArrow", "Lines", "A line draws with heads on both ends", 0.8f, [](MotionBuilder &b)
                { BuildArrow(b, Local(b, {{-160.0f, 0.0f}, {160.0f, 0.0f}}), false, true, false); }, MotionInput::Points);
            Add(list, "CurvedArrow", "Lines", "A curved arrow arcs from the first point to the last", 0.9f, [](MotionBuilder &b)
                {
                    const std::vector<glm::vec2> in = Local(b, {{-150.0f, -40.0f}, {150.0f, -40.0f}});
                    const glm::vec2 a = in.front(), c = in.back();
                    const glm::vec2 mid = (a + c) * 0.5f + glm::vec2(-(c - a).y, (c - a).x) * 0.45f;
                    std::vector<glm::vec2> pts;
                    for (int i = 0; i <= 32; ++i)
                    {
                        const float t = static_cast<float>(i) / 32.0f;
                        pts.push_back((1 - t) * (1 - t) * a + 2 * (1 - t) * t * mid + t * t * c);
                    }
                    BuildArrow(b, pts, false, false, false); }, MotionInput::Points);
            Add(list, "Connector", "Lines", "Two dots pop and a line joins them", 1.0f, [](MotionBuilder &b)
                {
                    const std::vector<glm::vec2> pts = Local(b, {{-150.0f, -30.0f}, {150.0f, 30.0f}});
                    const float r = b.WidthOr(5.0f) * 2.4f;
                    b.Pop(b.Fill(VectorPath::Circle(r), pts.front(), b.Accent, 0.01f), 0.0f, 0.35f);
                    b.Draw(b.Line(pts, b.Color, b.WidthOr(5.0f)), 0.25f, 0.5f);
                    b.Pop(b.Fill(VectorPath::Circle(r), pts.back(), b.Accent, 0.01f), 0.7f, 0.35f); }, MotionInput::Points);
            Add(list, "Laser", "Lines", "A bright beam flashes between two points", 0.7f, [](MotionBuilder &b)
                {
                    const std::vector<glm::vec2> pts = Local(b, {{-180.0f, 0.0f}, {180.0f, 0.0f}});
                    const entt::entity beam = b.Line({pts.front(), pts.back()}, b.ColorOr(Rose), b.WidthOr(8.0f));
                    b.Glow(beam, b.ColorOr(Rose), 2.5f, 1.0f);
                    b.Draw(beam, 0.0f, 0.15f, "easeOut");
                    b.To(beam, {One(TweenProperty::StrokeWidth, b.WidthOr(8.0f), 1.0f), AlphaT(1.0f, 0.0f)}, 0.25f, 0.4f, "easeIn"); }, MotionInput::Points);
            Add(list, "ZigZag", "Lines", "A zig-zag line draws", 0.9f, [](MotionBuilder &b)
                {
                    std::vector<glm::vec2> pts;
                    const float w = b.SizeOr(320.0f);
                    for (int i = 0; i <= 8; ++i)
                        pts.push_back({-w * 0.5f + w * static_cast<float>(i) / 8.0f, i % 2 ? 40.0f : -40.0f});
                    b.Draw(b.Line(Local(b, pts), b.Color, b.WidthOr(6.0f)), 0.0f, 0.9f); }, MotionInput::Points);
            Add(list, "Spiral", "Lines", "A spiral winds out from the center", 1.2f, [](MotionBuilder &b)
                {
                    std::vector<glm::vec2> pts;
                    const float r = b.SizeOr(110.0f);
                    for (int i = 0; i <= 200; ++i)
                    {
                        const float t = static_cast<float>(i) / 200.0f;
                        pts.push_back(Dir(t * 1080.0f) * (r * t));
                    }
                    b.Draw(b.Line(pts, b.Color, b.WidthOr(5.0f)), 0.0f, 1.2f, "easeInOut"); });
            Add(list, "SineWave", "Lines", "A sine wave draws across", 1.0f, [](MotionBuilder &b)
                {
                    std::vector<glm::vec2> pts;
                    const float w = b.SizeOr(340.0f);
                    for (int i = 0; i <= 120; ++i)
                    {
                        const float t = static_cast<float>(i) / 120.0f;
                        pts.push_back({-w * 0.5f + w * t, std::sin(t * 4.0f * Pi) * w * 0.15f});
                    }
                    const entt::entity wave = b.Line(pts, b.Color, b.WidthOr(5.0f));
                    b.Glow(wave, b.Color, 1.0f, 1.0f);
                    b.Draw(wave, 0.0f, 1.0f, "easeInOut"); });
            Add(list, "PolygonDraw", "Lines", "A closed shape through the points draws, then fills", 1.2f, [](MotionBuilder &b)
                {
                    const std::vector<glm::vec2> pts = Local(b, {{-120.0f, -80.0f}, {130.0f, -60.0f}, {80.0f, 90.0f}, {-90.0f, 70.0f}});
                    const entt::entity fill = b.Fill(VectorPath::Points(pts, true), {0, 0}, WithA(b.Color, 0.35f), -0.01f);
                    b.FadeIn(fill, 0.8f, 0.4f);
                    b.Draw(b.Line(pts, b.Color, b.WidthOr(5.0f), 0.0f, true), 0.0f, 0.9f); }, MotionInput::Points);
            Add(list, "Scribble", "Lines", "A quick hand-drawn scribble", 0.9f, [](MotionBuilder &b)
                {
                    std::vector<glm::vec2> pts;
                    const float w = b.SizeOr(260.0f);
                    for (int i = 0; i <= 60; ++i)
                    {
                        const float t = static_cast<float>(i) / 60.0f;
                        pts.push_back({-w * 0.5f + w * t + std::sin(t * 37.0f) * 14.0f, std::sin(t * 23.0f) * 26.0f + std::cos(t * 9.0f) * 10.0f});
                    }
                    b.Draw(b.Line(pts, b.ColorOr(Rose), b.WidthOr(4.0f)), 0.0f, 0.85f, "easeOut"); });

            static const struct
            {
                const char *Name;
                int From;
                int To;
            } morphs[] = {{"MorphCircleSquare", 0, 1}, {"MorphSquareStar", 1, 3}, {"MorphStarHeart", 3, 4}, {"MorphHexagonCircle", 5, 0}, {"MorphTriangleCircle", 2, 0}};
            for (const auto &m : morphs)
            {
                const int from = m.From, to = m.To;
                Add(list, m.Name, "Morph", std::string("A ") + ShapeKinds()[static_cast<size_t>(m.From)].Name + " pops in and morphs into a " + ShapeKinds()[static_cast<size_t>(m.To)].Name, 1.6f,
                    [from, to](MotionBuilder &b)
                    {
                        const float r = b.SizeOr(60.0f);
                        const entt::entity e = b.Fill(ShapeKinds()[static_cast<size_t>(from)].Path(r), {0, 0}, b.Color);
                        b.V(e).morphTarget = ShapeKinds()[static_cast<size_t>(to)].Path(r);
                        b.Pop(e, 0.0f, 0.4f);
                        b.To(e, {One(TweenProperty::Morph, 0.0f, 1.0f)}, 0.5f, 0.8f, "easeInOutCubic");
                        b.To(e, {One(TweenProperty::Rotation, 0.0f, 90.0f)}, 0.5f, 0.8f, "easeInOutCubic");
                    });
            }

            Add(list, "Axes", "Charts", "X and Y axes draw with ticks", 1.0f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    Axes(b, {s * 2.4f, s * 1.6f}, -s * 0.8f, 0.0f, true); });
            Add(list, "Grid", "Charts", "A grid fades in line by line", 1.1f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const float w = s * 2.4f, h = s * 1.6f;
                    for (int i = 0; i <= 8; ++i)
                    {
                        const float x = -w * 0.5f + w * static_cast<float>(i) / 8.0f;
                        b.Draw(b.Line({{x, -h * 0.5f}, {x, h * 0.5f}}, WithA(White, 0.18f), 1.5f), 0.04f * static_cast<float>(i), 0.5f);
                    }
                    for (int i = 0; i <= 6; ++i)
                    {
                        const float y = -h * 0.5f + h * static_cast<float>(i) / 6.0f;
                        b.Draw(b.Line({{-w * 0.5f, y}, {w * 0.5f, y}}, WithA(White, 0.18f), 1.5f), 0.2f + 0.04f * static_cast<float>(i), 0.5f);
                    } });
            Add(list, "NumberLine", "Charts", "A number line with ticks and labels", 1.2f, [](MotionBuilder &b)
                {
                    const float w = b.SizeOr(360.0f);
                    b.Draw(b.Line({{-w * 0.5f - 10.0f, 0.0f}, {w * 0.5f + 10.0f, 0.0f}}, White, 3.0f), 0.0f, 0.5f);
                    for (int i = -4; i <= 4; ++i)
                    {
                        const float x = w * 0.5f * static_cast<float>(i) / 4.0f;
                        const float t = 0.3f + 0.05f * static_cast<float>(i + 4);
                        b.Pop(b.Line({{x, -8.0f}, {x, 8.0f}}, White, 2.5f), t, 0.25f);
                        b.FadeIn(b.Label(std::to_string(i), {x, -26.0f}, 18.0f, WithA(White, 0.8f)), t + 0.05f, 0.3f);
                    } });

            const auto plotFn = [](float x)
            { return std::sin(x); };
            Add(list, "Plot", "Charts", "Axes, then the curve draws (points, values or fn)", 1.5f, [plotFn](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const Chart c{s * 2.4f, s * 1.6f};
                    const Samples data = FitSamples(b, c, plotFn, -Pi * 2.0f, Pi * 2.0f);
                    Axes(b, c, data.OriginY, 0.0f, true);
                    b.Draw(b.Line(data.Points, b.Color, b.WidthOr(4.0f), 0.01f), 0.5f, 1.0f, "easeInOut"); }, MotionInput::Values);
            Add(list, "PlotTracer", "Charts", "The curve draws while a glowing dot rides its tip", 1.5f, [plotFn](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const Chart c{s * 2.4f, s * 1.6f};
                    const Samples data = FitSamples(b, c, plotFn, -Pi * 2.0f, Pi * 2.0f);
                    Axes(b, c, data.OriginY, 0.0f, false);
                    b.Draw(b.Line(data.Points, b.Color, b.WidthOr(4.0f), 0.01f), 0.5f, 1.0f, "easeInOut");
                    const entt::entity dot = b.Fill(VectorPath::Circle(7.0f), data.Points.front(), b.Accent, 0.03f);
                    b.Glow(dot, b.Accent, 2.0f, 1.0f);
                    b.Pop(dot, 0.4f, 0.2f);
                    b.Along(dot, data.Points, 0.5f, 1.0f, "easeInOut"); }, MotionInput::Values);
            Add(list, "PlotArea", "Charts", "The curve draws and the area under it fills", 1.8f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const Chart c{s * 2.4f, s * 1.6f};
                    const Samples data = FitSamples(b, c, [](float x) { return 1.2f + std::sin(x) * 0.8f + 0.3f * std::sin(3.0f * x); }, 0.0f, Pi * 2.0f);
                    Axes(b, c, data.OriginY, 0.0f, false);
                    std::vector<glm::vec2> area = data.Points;
                    area.push_back({data.Points.back().x, data.OriginY});
                    area.push_back({data.Points.front().x, data.OriginY});
                    const entt::entity fill = b.Fill(VectorPath::Points(area, true), {0, 0}, WithA(b.Color, 0.35f), 0.0f);
                    b.FadeIn(fill, 1.3f, 0.5f);
                    b.Draw(b.Line(data.Points, b.Color, b.WidthOr(4.0f), 0.01f), 0.5f, 0.9f, "easeInOut"); }, MotionInput::Values);
            Add(list, "Parabola", "Charts", "y = x squared draws over axes", 1.5f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const Chart c{s * 2.4f, s * 1.6f};
                    const Samples data = FitSamples(b, c, [](float x) { return x * x; }, -2.0f, 2.0f);
                    Axes(b, c, data.OriginY, 0.0f, true);
                    b.Draw(b.Line(data.Points, b.Color, b.WidthOr(4.0f), 0.01f), 0.5f, 1.0f, "easeInOut"); }, MotionInput::Values);
            Add(list, "Riemann", "Charts", "A curve, then rectangles under it grow one by one", 2.0f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const Chart c{s * 2.4f, s * 1.6f};
                    const auto fn = [](float x) { return 0.6f + x * x * 0.25f; };
                    const Samples data = FitSamples(b, c, fn, 0.0f, 3.0f);
                    Axes(b, c, data.OriginY, 0.0f, false);
                    b.Draw(b.Line(data.Points, b.Color, b.WidthOr(4.0f), 0.02f), 0.4f, 0.8f, "easeInOut");
                    const int bars = 10;
                    const float bw = c.W / static_cast<float>(bars);
                    for (int i = 0; i < bars; ++i)
                    {
                        const size_t idx = std::min(data.Points.size() - 1, static_cast<size_t>((static_cast<float>(i) + 0.5f) / static_cast<float>(bars) * static_cast<float>(data.Points.size() - 1)));
                        const float h = data.Points[idx].y - data.OriginY;
                        const entt::entity bar = b.Fill(Bar(bw - 3.0f, h, 0.0f), {-c.W * 0.5f + bw * (static_cast<float>(i) + 0.5f), data.OriginY}, WithA(b.Accent, 0.55f), 0.0f);
                        b.Tf(bar).scale.y = 0.0f;
                        b.To(bar, {One(TweenProperty::ScaleY, 0.0f, 1.0f)}, 1.1f + 0.07f * static_cast<float>(i), 0.4f, "easeOutBack");
                    } });

            const auto bars = [](MotionBuilder &b, bool labels, bool thin)
            {
                const float s = b.SizeOr(140.0f);
                const Chart c{s * 2.4f, s * 1.6f};
                std::vector<float> values = b.ValuesOr(thin ? std::vector<float>{2, 4, 7, 9, 12, 10, 8, 6, 4, 3, 2, 1} : std::vector<float>{3, 5, 2, 6, 4});
                float hi = 0.0f;
                for (float v : values)
                    hi = std::max(hi, v);
                hi = std::max(hi, 1e-4f);
                Axes(b, c, -c.H * 0.5f, 0.0f, false);
                const float slot = c.W / static_cast<float>(values.size());
                for (size_t i = 0; i < values.size(); ++i)
                {
                    const float h = values[i] / hi * c.H * 0.92f;
                    const glm::vec2 base{-c.W * 0.5f + slot * (static_cast<float>(i) + 0.5f), -c.H * 0.5f};
                    const entt::entity bar = b.Fill(Bar(slot * (thin ? 0.86f : 0.62f), h, thin ? 0.0f : 6.0f), base, PaletteColor(thin ? 0 : i, b.Color, b.Accent), 0.0f);
                    b.Tf(bar).scale.y = 0.0f;
                    const float t = 0.4f + (thin ? 0.04f : 0.1f) * static_cast<float>(i);
                    b.To(bar, {One(TweenProperty::ScaleY, 0.0f, 1.0f)}, t, 0.6f, "easeOutBack");
                    if (labels)
                    {
                        char text[32];
                        std::snprintf(text, sizeof(text), "%g", values[i]);
                        b.Pop(b.Label(text, base + glm::vec2(0.0f, h + 18.0f), 18.0f, White, 0.02f), t + 0.4f, 0.3f);
                    }
                }
            };
            Add(list, "BarChart", "Charts", "Bars grow one after another (values)", 1.4f, [bars](MotionBuilder &b) { bars(b, false, false); }, MotionInput::Values);
            Add(list, "BarChartLabels", "Charts", "Bars grow and their values pop on top (values)", 1.7f, [bars](MotionBuilder &b) { bars(b, true, false); }, MotionInput::Values);
            Add(list, "Histogram", "Charts", "Thin bars rise in a wave (values)", 1.5f, [bars](MotionBuilder &b) { bars(b, false, true); }, MotionInput::Values);
            Add(list, "LineChart", "Charts", "A line through the values draws and its points pop", 1.6f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const Chart c{s * 2.4f, s * 1.6f};
                    std::vector<float> values = b.ValuesOr({2, 4, 3, 6, 5, 8, 7});
                    float hi = 0.0f;
                    for (float v : values)
                        hi = std::max(hi, v);
                    Axes(b, c, -c.H * 0.5f, 0.0f, false);
                    std::vector<glm::vec2> pts;
                    for (size_t i = 0; i < values.size(); ++i)
                        pts.push_back(c.Map(static_cast<float>(i), values[i], -0.5f, static_cast<float>(values.size()) - 0.5f, 0.0f, hi * 1.1f));
                    b.Draw(b.Line(pts, b.Color, b.WidthOr(4.0f)), 0.4f, 0.9f, "easeInOut");
                    for (size_t i = 0; i < pts.size(); ++i)
                        b.Pop(b.Fill(VectorPath::Circle(7.0f), pts[i], b.Accent, 0.02f), 0.4f + 0.9f * static_cast<float>(i) / static_cast<float>(pts.size() - 1), 0.3f); }, MotionInput::Values);
            Add(list, "ScatterPlot", "Charts", "Points pop in and a trend line draws (points)", 1.7f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const Chart c{s * 2.4f, s * 1.6f};
                    Axes(b, c, -c.H * 0.5f, 0.0f, false);
                    std::vector<glm::vec2> pts = b.O.Points;
                    if (pts.size() < 2)
                        for (int i = 0; i < 18; ++i)
                        {
                            const float x = static_cast<float>(i) / 17.0f;
                            const float y = 0.15f + 0.7f * x + 0.12f * std::sin(static_cast<float>(i) * 2.7f);
                            pts.push_back(c.Map(x, y, 0.0f, 1.0f, 0.0f, 1.0f));
                        }
                    for (size_t i = 0; i < pts.size(); ++i)
                        b.Pop(b.Fill(VectorPath::Circle(6.0f), pts[i], PaletteColor(i % 2, b.Color, b.Accent), 0.01f), 0.3f + 0.04f * static_cast<float>(i), 0.3f);
                    glm::vec2 mean{0.0f};
                    for (glm::vec2 p : pts)
                        mean += p;
                    mean /= static_cast<float>(pts.size());
                    float num = 0.0f, den = 0.0f;
                    for (glm::vec2 p : pts)
                    {
                        num += (p.x - mean.x) * (p.y - mean.y);
                        den += (p.x - mean.x) * (p.x - mean.x);
                    }
                    const float slope = den > 0.0f ? num / den : 0.0f;
                    const float x0 = -c.W * 0.5f, x1 = c.W * 0.5f;
                    b.Draw(b.Line({{x0, mean.y + slope * (x0 - mean.x)}, {x1, mean.y + slope * (x1 - mean.x)}}, WithA(White, 0.7f), 3.0f, 0.02f), 1.1f, 0.5f); }, MotionInput::Points);
            Add(list, "PieChart", "Charts", "Pie slices pop in one by one (values)", 1.4f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(110.0f);
                    std::vector<float> values = b.ValuesOr({35, 25, 20, 12, 8});
                    float total = 0.0f;
                    for (float v : values)
                        total += std::max(v, 0.0f);
                    total = std::max(total, 1e-4f);
                    float angle = 90.0f;
                    for (size_t i = 0; i < values.size(); ++i)
                    {
                        const float sweep = std::max(values[i], 0.0f) / total * 360.0f;
                        const entt::entity slice = b.Fill(Pie(r, angle, angle - sweep), {0, 0}, PaletteColor(i, b.Color, b.Accent));
                        b.Pop(slice, 0.12f * static_cast<float>(i), 0.45f);
                        angle -= sweep;
                    } }, MotionInput::Values);
            Add(list, "DonutChart", "Charts", "A donut chart draws around segment by segment (values)", 1.6f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(90.0f);
                    std::vector<float> values = b.ValuesOr({40, 30, 20, 10});
                    float total = 0.0f;
                    for (float v : values)
                        total += std::max(v, 0.0f);
                    total = std::max(total, 1e-4f);
                    float angle = 90.0f;
                    float t = 0.0f;
                    for (size_t i = 0; i < values.size(); ++i)
                    {
                        const float sweep = std::max(values[i], 0.0f) / total * 360.0f;
                        const float d = 1.2f * sweep / 360.0f;
                        const entt::entity arc = b.Line(Arc({0, 0}, r, angle - 1.0f, angle - sweep + 1.0f, std::max(4, static_cast<int>(sweep / 4.0f))), PaletteColor(i, b.Color, b.Accent), b.WidthOr(r * 0.35f));
                        b.V(arc).cap = StrokeCap::Butt;
                        b.Draw(arc, t, d, "linear");
                        t += d;
                        angle -= sweep;
                    } }, MotionInput::Values);
            Add(list, "VectorArrow", "Charts", "A math vector arrow from the origin to points[1] with a label", 1.1f, [](MotionBuilder &b)
                {
                    const float s = b.SizeOr(140.0f);
                    const Chart c{s * 2.4f, s * 1.6f};
                    Axes(b, c, 0.0f, 0.0f, false);
                    const glm::vec2 tip = b.O.Points.empty() ? glm::vec2(s * 0.9f, s * 0.55f) : b.O.Points.back();
                    BuildArrow(b, {{0.0f, 0.0f}, tip}, false, false, false);
                    b.Pop(b.Label(b.TextOr("v"), tip + glm::normalize(tip) * 26.0f, 24.0f, b.Color, 0.03f), 0.8f, 0.3f); }, MotionInput::Points);
            Add(list, "AngleArc", "Charts", "Two rays and the angle between them, labeled", 1.3f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(150.0f);
                    const float angle = b.ValuesOr({50.0f})[0];
                    b.Draw(b.Line({{0, 0}, {r, 0.0f}}, White, 3.0f), 0.0f, 0.4f);
                    b.Draw(b.Line({{0, 0}, Dir(angle) * r}, White, 3.0f), 0.2f, 0.4f);
                    b.Draw(b.Line(Arc({0, 0}, r * 0.35f, 0.0f, angle, 32), b.Accent, 3.0f), 0.6f, 0.4f);
                    b.Pop(b.Label(b.TextOr("θ"), Dir(angle * 0.5f) * r * 0.55f, 24.0f, b.Accent), 0.9f, 0.3f); }, MotionInput::Values);
            Add(list, "UnitCircle", "Charts", "A circle with a radius that sweeps around", 2.2f, [](MotionBuilder &b)
                {
                    const float r = b.SizeOr(110.0f);
                    Axes(b, {r * 2.6f, r * 2.6f}, 0.0f, 0.0f, false);
                    b.Draw(b.Line(Arc({0, 0}, r, 0.0f, 360.0f, 96), b.Color, 3.0f, 0.01f), 0.3f, 0.7f);
                    const entt::entity arm = b.Node("Radius", {0, 0}, 0.02f);
                    b.Line({{0, 0}, {r, 0}}, b.Accent, 3.0f, 0.0f, false, arm);
                    b.Fill(VectorPath::Circle(7.0f), {r, 0.0f}, b.Accent, 0.01f, arm);
                    b.FadeIn(arm, 1.0f, 0.2f);
                    b.To(arm, {One(TweenProperty::Rotation, 0.0f, 360.0f)}, 1.0f, 1.2f, "easeInOut"); });

            AddMore(list);
            return list;
        }

        std::string Lower(std::string s)
        {
            for (char &c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }
    }

    namespace MotionPresets
    {
        const std::vector<MotionPreset> &All()
        {
            static const std::vector<MotionPreset> presets = BuildAll();
            return presets;
        }

        const MotionPreset *Find(const std::string &name)
        {
            const std::string key = Lower(name);
            for (const MotionPreset &preset : All())
                if (Lower(preset.Name) == key)
                    return &preset;
            return nullptr;
        }

        std::vector<std::string> Categories()
        {
            std::vector<std::string> out;
            for (const MotionPreset &preset : All())
                if (std::find(out.begin(), out.end(), preset.Category) == out.end())
                    out.push_back(preset.Category);
            return out;
        }

        entt::entity Play(Scene &scene, const MotionPreset &preset, const MotionOptions &options, float *length)
        {
            MotionBuilder builder(scene, preset, options);
            if (preset.Build)
                preset.Build(builder);
            const float total = options.Delay + builder.End * builder.TimeScale;
            if (length)
                *length = total;
            if (options.Hold >= 0.0f)
                scene.GetRegistry().emplace_or_replace<LifetimeComponent>(builder.Root).remaining = total + options.Hold;
            return builder.Root;
        }

        std::string Example(const MotionPreset &preset)
        {
            switch (preset.Input)
            {
            case MotionInput::Points:
                return "Motion." + preset.Name + "({{-150, 0}, {0, 80}, {150, 0}})";
            case MotionInput::Values:
                return "Motion." + preset.Name + "({x = 0, y = 0, values = {3, 5, 2, 6}})";
            case MotionInput::Text:
                return "Motion." + preset.Name + "(\"Hello\", {x = 0, y = 0})";
            default:
                return "Motion." + preset.Name + "(0, 0)";
            }
        }

        std::vector<float> SampleFunction(const std::function<float(float)> &fn, float from, float to, int count)
        {
            std::vector<float> out;
            count = std::max(2, count);
            for (int i = 0; i < count; ++i)
            {
                const float x = from + (to - from) * static_cast<float>(i) / static_cast<float>(count - 1);
                float y = fn ? fn(x) : 0.0f;
                if (!std::isfinite(y))
                    y = 0.0f;
                out.push_back(y);
            }
            return out;
        }
    }
}
