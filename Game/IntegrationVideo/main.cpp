#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Window.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/GLRenderingAPI.h>
#include <Engine/Rendering/ImageWriter.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Vector/VectorShape.h>

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

using SIMPEngine::Renderer;
using SIMPEngine::TextAlign;

namespace
{
    using Rgb = glm::vec3;
    using Pts = std::vector<glm::vec2>;
    using Ease = float (*)(float);

    const Rgb BG{14, 17, 30}, PANEL{25, 30, 52}, INK{236, 241, 252}, MUTED{138, 148, 176}, CYAN{88, 206, 255};
    const Rgb TEAL{62, 214, 176}, CORAL{255, 126, 96}, GOLD{255, 206, 96}, VIOLET{172, 142, 255};

    constexpr float A = 0, B = 5, OX = -675, OY = -330, SX = 270, SY = 180, BX = 0, BY = 1440, TX = 0, TY = 1560;
    constexpr int STAGES[] = {4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
    constexpr int NSTAGES = 10;
    constexpr float WIN_B = 67.0f * (B - A) / 256.0f;
    constexpr float SPLIT_DUR[] = {1.7f, 1.4f, 1.15f, 1.0f, 0.9f, 0.8f, 1.2f, 1.1f, 1.0f};
    constexpr float SPLIT_HOLD[] = {0.9f, 0.6f, 0.5f, 0.4f, 0.4f, 1.0f, 0.6f, 0.5f, 1.0f};
    constexpr int GLYPH_N = 110;
    constexpr float PI = 3.14159265358979f;

    float f(float x) { return 1.8f + 1.2f * std::sin(1.2f * x) + 0.25f * x; }
    float Antiderivative(float x) { return 1.8f * x - std::cos(1.2f * x) + 0.125f * x * x; }
    const float TRUE_AREA = Antiderivative(B) - Antiderivative(A);

    float px(float x) { return OX + SX * x; }
    float py(float y) { return OY + SY * y; }
    float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
    float lerp(float a, float b, float t) { return a + (b - a) * t; }
    float prog(float t, float a, float b) { return clamp01((t - a) / (b - a)); }
    Rgb mix(Rgb a, Rgb b, float t) { return a + (b - a) * t; }

    namespace ease
    {
        float inOutCubic(float u)
        {
            if (u < 0.5f)
                return 4 * u * u * u;
            float v = -2 * u + 2;
            return 1 - v * v * v / 2;
        }
        float outCubic(float u)
        {
            float v = 1 - u;
            return 1 - v * v * v;
        }
        float inOutSine(float u) { return -(std::cos(PI * u) - 1) / 2; }
        float outBack(float u)
        {
            const float c1 = 1.70158f, c3 = c1 + 1;
            float v = u - 1;
            return 1 + c3 * v * v * v + c1 * v * v;
        }
    }

    struct Span
    {
        float a = 0, b = 0;
    };
    std::unordered_map<std::string, Span> T;
    float TOTAL = 0;
    std::vector<float> SUMS;

    const Span &S(const std::string &name) { return T.at(name); }
    float seg(float t, const char *name) { return prog(t, S(name).a, S(name).b); }
    std::string Split(int i) { return "split" + std::to_string(i); }
    std::string Hold(int i) { return "hold" + std::to_string(i); }

    void BuildTimeline()
    {
        float cur = 0;
        auto add = [&](const std::string &name, float dur)
        {
            T[name] = {cur, cur + dur};
            cur += dur;
        };
        add("intro", 4.6f);
        add("pan", 2.4f);
        add("axes", 2.2f);
        add("curve", 5.4f);
        add("pull", 1.5f);
        add("area", 4.6f);
        add("big", 3.4f);
        add("bigIn", 1.5f);
        add("bigNote", 4.4f);
        add("bigOut", 1.3f);
        add("bigSum", 2.6f);
        for (int i = 1; i <= 6; ++i)
        {
            add(Split(i), SPLIT_DUR[i - 1]);
            add(Hold(i), SPLIT_HOLD[i - 1]);
        }
        add("diveIn", 1.8f);
        for (int i = 7; i <= 9; ++i)
        {
            add(Split(i), SPLIT_DUR[i - 1]);
            add(Hold(i), SPLIT_HOLD[i - 1]);
        }
        add("diveOut", 1.8f);
        add("smooth", 3.4f);
        add("panUp", 2.4f);
        add("fSum", 3.8f);
        add("fExplain", 3.2f);
        add("fThin", 3.4f);
        add("fMorph", 3.4f);
        add("fResult", 4.2f);
        add("outro", 3.4f);
        add("fade", 1.0f);
        TOTAL = cur;

        for (int k = 0; k < NSTAGES; ++k)
        {
            float dx = (B - A) / STAGES[k], s = 0;
            for (int i = 0; i < STAGES[k]; ++i)
                s += f(A + i * dx);
            SUMS.push_back(s * dx);
        }
    }

    float g_z = 0;
    void SetZ(float z) { g_z = z; }
    float a255(float a) { return std::floor(255 * clamp01(a) + 0.5f); }
    Uint8 Byte(float v) { return static_cast<Uint8>(std::clamp(v, 0.0f, 255.0f)); }
    SDL_Color Col(Rgb c, float a = 1) { return {Byte(c.r), Byte(c.g), Byte(c.b), Byte(a255(a))}; }
    glm::vec4 Vec(SDL_Color c) { return glm::vec4(c.r, c.g, c.b, c.a) / 255.0f; }

    void line(float x1, float y1, float x2, float y2, float w, Rgb c, float a = 1) { Renderer::DrawCapsuleLine(x1, y1, x2, y2, w, Col(c, a), 1.0f, g_z); }
    void circle(float x, float y, float r, Rgb c, float a = 1) { Renderer::DrawCircle(x, y, r, Col(c, a), 1.0f, g_z); }
    void ring(float x, float y, float r, float th, Rgb c, float a = 1) { Renderer::DrawRing(x, y, r, th, Col(c, a), 1.0f, g_z); }
    void rect(float x, float y, float w, float h, Rgb c, float a = 1) { Renderer::DrawQuad(x, y, w, h, 0, Col(c, a), true, g_z); }
    void rrect(float x, float y, float w, float h, float r, Rgb c, float a = 1) { Renderer::DrawRoundedRect(x, y, w * 0.5f, h * 0.5f, r, Col(c, a), 1.0f, g_z); }

    void pline(const Pts &pts, float w, Rgb c, float a, bool closed)
    {
        if (pts.size() < 2)
            return;
        SIMPEngine::StrokeStyle style;
        style.Width = w;
        SIMPEngine::VectorShapes::DrawPolyline(pts, closed, style, Vec(Col(c, a)), g_z);
    }

    void poly(const Pts &pts, Rgb c, float a) { SIMPEngine::VectorShapes::DrawPolygon(pts, Vec(Col(c, a)), g_z); }

    void text(const std::string &str, float x, float cy, float size, Rgb c, float a = 1, TextAlign align = TextAlign::Center)
    {
        if (a <= 0.004f)
            return;
        Renderer::DrawText(str, x, cy + size * 0.52f, size, Col(c, a), align, 0.0f, g_z);
    }

    float tw(const std::string &str, float size) { return static_cast<float>(str.size()) * size * 0.56f; }

    std::string Fmt(const char *fmt, double v)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), fmt, v);
        return buf;
    }

    void dashed(float x1, float y1, float x2, float y2, float w, float dash, float gap, Rgb c, float a, float p = 1)
    {
        float len = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
        if (len < 0.5f)
            return;
        float reach = len * p, period = dash + gap;
        if (reach / period > 90)
        {
            float k = reach / period / 90;
            dash *= k;
            gap *= k;
            period *= k;
        }
        float ux = (x2 - x1) / len, uy = (y2 - y1) / len;
        for (float d = 0; d < reach; d += period)
        {
            float e = std::min(d + dash, reach);
            line(x1 + ux * d, y1 + uy * d, x1 + ux * e, y1 + uy * e, w, c, a);
        }
    }

    void arrow(float x1, float y1, float x2, float y2, float w, Rgb c, float a)
    {
        line(x1, y1, x2, y2, w, c, a);
        float ang = std::atan2(y2 - y1, x2 - x1), hs = w * 4.2f;
        for (float s : {1.0f, -1.0f})
        {
            float b = ang + PI + s * 0.5f;
            line(x2, y2, x2 + std::cos(b) * hs, y2 + std::sin(b) * hs, w, c, a);
        }
    }

    void deltaGlyph(float cx, float cy, float d, Rgb c, float a)
    {
        pline({{cx - 0.4f * d, cy - 0.36f * d}, {cx + 0.4f * d, cy - 0.36f * d}, {cx, cy + 0.4f * d}}, d * 0.12f, c, a, true);
    }

    void deltaX(float cx, float cy, float s, Rgb c, float a)
    {
        float left = cx - 0.72f * s;
        deltaGlyph(left + 0.38f * s, cy, s * 0.82f, c, a);
        text("x", left + 0.82f * s, cy, s, c, a, TextAlign::Left);
    }

    void infinity(float cx, float cy, float s, Rgb c, float a, float w)
    {
        Pts pts;
        for (int i = 0; i <= 48; ++i)
        {
            float t = i / 48.0f * PI * 2, d = 1 + std::sin(t) * std::sin(t);
            pts.push_back({cx + s * std::cos(t) / d, cy + s * std::sin(t) * std::cos(t) / d});
        }
        pline(pts, w, c, a, true);
    }

    void approx(float cx, float cy, float s, float flat, Rgb c, float a)
    {
        for (float off : {0.17f, -0.17f})
        {
            Pts pts;
            for (int i = 0; i <= 16; ++i)
            {
                float u = i / 16.0f;
                pts.push_back({cx + (u - 0.5f) * 0.9f * s, cy + off * s + (1 - flat) * 0.1f * s * std::sin(u * PI * 2)});
            }
            pline(pts, s * 0.075f, c, a, false);
        }
    }

    Pts resample(const Pts &pts, int n)
    {
        std::vector<float> lens{0};
        float total = 0;
        for (size_t i = 1; i < pts.size(); ++i)
        {
            total += glm::length(pts[i] - pts[i - 1]);
            lens.push_back(total);
        }
        Pts out;
        size_t j = 1;
        for (int k = 0; k < n; ++k)
        {
            float target = total * k / (n - 1);
            while (j < pts.size() - 1 && lens[j] < target)
                ++j;
            float span = lens[j] - lens[j - 1];
            float u = span > 0 ? clamp01((target - lens[j - 1]) / span) : 0;
            out.push_back(pts[j - 1] + (pts[j] - pts[j - 1]) * u);
        }
        return out;
    }

    void bezier(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, int n, Pts &out)
    {
        for (int i = 0; i <= n; ++i)
        {
            float t = static_cast<float>(i) / n, m = 1 - t;
            out.push_back(m * m * m * p0 + 3 * m * m * t * p1 + 3 * m * t * t * p2 + t * t * t * p3);
        }
    }

    Pts SIGMA_R, INTEG_R, CURVE_FULL, AREA_FULL;

    Pts glyphPoints(float cx, float cy, float H, float m, float trim)
    {
        Pts out;
        float wide = lerp(0.74f, 0.5f, m);
        int last = std::max(2, static_cast<int>(std::floor(GLYPH_N * trim + 0.5f)));
        for (int i = 0; i < last; ++i)
        {
            glm::vec2 p = SIGMA_R[i] + (INTEG_R[i] - SIGMA_R[i]) * m;
            out.push_back({cx + (p.x - 0.5f) * H * wide, cy + (p.y - 0.5f) * H});
        }
        return out;
    }

    Pts curveFlat(float xr, int np)
    {
        Pts out;
        float span = B - A;
        int steps = np - 1;
        int k = static_cast<int>(std::floor((xr - A) / span * steps + 1e-9f));
        for (int i = 0; i <= k; ++i)
        {
            float x = A + span * i / steps;
            out.push_back({px(x), py(f(x))});
        }
        if (xr - (A + span * k / steps) > 1e-6f)
            out.push_back({px(xr), py(f(xr))});
        return out;
    }

    Pts areaPoly(float xr)
    {
        Pts pts{{px(A), OY}};
        const Pts c = xr >= B - 1e-6f ? AREA_FULL : curveFlat(xr, 121);
        pts.insert(pts.end(), c.begin(), c.end());
        pts.push_back({px(xr), OY});
        return pts;
    }

    void BuildShapes()
    {
        Pts sigma{{0.95f, 0.88f}, {0.95f, 1.0f}, {0.05f, 1.0f}, {0.52f, 0.5f}, {0.05f, 0.0f}, {0.95f, 0.0f}, {0.95f, 0.12f}};
        Pts integ;
        bezier({0.06f, 0.10f}, {0.10f, -0.04f}, {0.38f, -0.04f}, {0.42f, 0.22f}, 18, integ);
        bezier({0.42f, 0.22f}, {0.47f, 0.40f}, {0.53f, 0.60f}, {0.58f, 0.78f}, 6, integ);
        bezier({0.58f, 0.78f}, {0.62f, 1.04f}, {0.90f, 1.04f}, {0.94f, 0.90f}, 18, integ);
        SIGMA_R = resample(sigma, GLYPH_N);
        INTEG_R = resample(integ, GLYPH_N);
        CURVE_FULL = curveFlat(B, 361);
        AREA_FULL = curveFlat(B, 121);
    }

    struct Pose
    {
        float x, y, z;
    };

    Pose penPose(float u)
    {
        float xr = A + (B - A) * ease::inOutSine(u);
        return {0.78f * px(xr), -190 + 0.25f * (py(f(xr)) + 190), lerp(0.86f, 0.92f, u)};
    }

    struct PathEntry
    {
        const char *name;
        Pose from, to;
        Ease curve;
        float bump;
        bool pen;
    };

    std::vector<PathEntry> PATH;

    void BuildPath()
    {
        const Pose WIDE{0, -75, 1.02f}, WIDE2{-20, -65, 1.08f};
        const Pose SLICE{px(0.62f) + 10, -285, 0.6f}, SLICE2{px(0.62f) - 10, -280, 0.57f};
        const Pose DIVE{px(0.55f), py(f(0.55f)), 0.18f};
        const Pose BOARD{BX, BY, 1.0f}, BOARD_ZOOM{BX, BY + 110, 0.8f};
        PATH = {
            {"intro", {TX, TY - 40, 1.38f}, {TX, TY - 40, 1.14f}, ease::outCubic, 0, false},
            {"pan", {TX, TY - 40, 1.14f}, penPose(0), ease::inOutCubic, 0.3f, false},
            {"curve", {}, {}, nullptr, 0, true},
            {"pull", penPose(1), WIDE, ease::inOutCubic, 0, false},
            {"area", WIDE, WIDE2, ease::inOutSine, 0, false},
            {"big", WIDE2, WIDE, ease::inOutSine, 0, false},
            {"bigIn", WIDE, SLICE, ease::inOutCubic, 0, false},
            {"bigNote", SLICE, SLICE2, ease::inOutSine, 0, false},
            {"bigOut", SLICE2, WIDE, ease::inOutCubic, 0, false},
            {"diveIn", WIDE, DIVE, ease::inOutCubic, 0, false},
            {"diveOut", DIVE, WIDE, ease::inOutCubic, 0, false},
            {"smooth", WIDE, WIDE2, ease::inOutSine, 0, false},
            {"panUp", WIDE2, BOARD, ease::inOutCubic, 0.35f, false},
            {"fMorph", BOARD, BOARD_ZOOM, ease::inOutCubic, 0, false},
            {"fResult", BOARD_ZOOM, BOARD, ease::inOutCubic, 0, false}};
    }

    struct Caption
    {
        const char *from, *to, *title, *body;
    };

    const Caption CAPS[] = {
        {"axes", "pull", "STEP 1  -  A NON-LINEAR CURVE", "Its height f(x) changes at every point along x"},
        {"area", "area", "THE QUESTION", "How much area lies under the curve, between a and b?"},
        {"big", "big", "STEP 2  -  BIG SLICES", "Cut the area into a few wide slices"},
        {"bigIn", "bigIn", "ONE SLICE", "Each slice is a rectangle"},
        {"bigNote", "bigNote", "ONE SLICE", "Area of a slice = its height x its width"},
        {"bigOut", "bigSum", "STEP 2  -  ADD THEM UP", "A rough estimate: the rectangles miss the curve"},
        {"split1", "hold3", "STEP 3  -  THINNER SLICES", "Cut every slice in half, then again..."},
        {"split4", "hold6", "STEP 3  -  THINNER SLICES", "More slices, thinner slices, smaller error"},
        {"diveIn", "diveOut", "ZOOM IN", "Even at 256 slices the tops are tiny steps - keep cutting"},
        {"smooth", "smooth", "THE LIMIT", "Infinitely thin slices: the staircase becomes the curve"},
        {"panUp", "fSum", "STEP 4  -  THE FORMULA", "Write down the sum of all the slices"},
        {"fExplain", "fExplain", "EACH TERM", "The height of one slice times its width"},
        {"fThin", "fThin", "THE LIMIT", "Width shrinks toward zero, slice count toward infinity"},
        {"fMorph", "fMorph", "THE INTEGRAL", "The sum turns into the integral"},
        {"fResult", "fResult", "RESULT", "For our curve, from 0 to 5, the exact area is"}};

    struct Stage
    {
        int from, to;
        float u;
    };

    class Integration
    {
    public:
        float t = 0, cx = 0, cy = 0, z = 1;

        Pose CameraPose(float time) const
        {
            Pose last{TX, TY - 40, 1.38f};
            for (const PathEntry &e : PATH)
            {
                const Span &s = S(e.name);
                if (time < s.a)
                    return last;
                float u = prog(time, s.a, s.b);
                Pose p;
                if (e.pen)
                    p = penPose(u);
                else
                {
                    float k = e.curve(u);
                    p = {lerp(e.from.x, e.to.x, k), lerp(e.from.y, e.to.y, k), lerp(e.from.z, e.to.z, k)};
                    if (e.bump != 0)
                        p.z += e.bump * std::sin(PI * k);
                }
                if (time <= s.b)
                    return p;
                last = e.pen ? penPose(1) : e.to;
            }
            return last;
        }

        Stage StageAt(float time) const
        {
            for (int i = 1; i <= NSTAGES - 1; ++i)
            {
                const Span &s = S(Split(i)), &h = S(Hold(i));
                if (time < s.a)
                    return {i, i, 1};
                if (time < s.b)
                    return {i, i + 1, ease::inOutCubic(prog(time, s.a, s.b))};
                if (time < h.b)
                    return {i + 1, i + 1, 1};
            }
            return {NSTAGES, NSTAGES, 1};
        }

        void ApplyCamera()
        {
            Pose p = CameraPose(t);
            cx = p.x + 5 * p.z * std::sin(t * 0.55f);
            cy = p.y + 4 * p.z * std::sin(t * 0.71f + 1.3f);
            z = p.z * (1 + 0.004f * std::sin(t * 0.43f));
        }

        void DrawBackdrop()
        {
            SetZ(-100);
            rect(cx, cy, 9000 * z, 9000 * z, BG, 1);
            SetZ(-90);
            const float s = 90, hw = 1400 * z, hh = 620 * z, w = 1.2f * z;
            auto major = [&](float v)
            {
                int k = static_cast<int>(std::floor(v / s + 0.5f));
                return ((k % 5) + 5) % 5 == 0;
            };
            for (float x = std::floor((cx - hw) / s) * s; x <= cx + hw; x += s)
                line(x, cy - hh, x, cy + hh, w, INK, major(x) ? 0.07f : 0.035f);
            for (float y = std::floor((cy - hh) / s) * s; y <= cy + hh; y += s)
                line(cx - hw, y, cx + hw, y, w, INK, major(y) ? 0.07f : 0.035f);
        }

        void DrawTitle()
        {
            if (t > S("axes").a)
                return;
            SetZ(-50);
            const float base = TY - 330;
            for (int i = 0; i <= 27; ++i)
            {
                float x = -840.0f + i * 62;
                float u = ease::outCubic(prog(t, 0.4f + i * 0.05f, 1.4f + i * 0.05f));
                float h = (90 + 150 * (f((x + 840) / 1680 * 5) - 1.4f) / 2.2f) * u;
                rect(x, base + h * 0.5f, 46, h, mix(CORAL, GOLD, i / 27.0f), 0.14f);
            }
            SetZ(10);
            float drawn = ease::inOutSine(prog(t, 0.3f, 2.0f));
            Pts pts = glyphPoints(0, TY + 250, 330, 1, drawn);
            pline(pts, 22 * z, VIOLET, 0.25f, false);
            pline(pts, 12, mix(VIOLET, GOLD, drawn), 1, false);
            float ta = prog(t, 1.2f, 2.0f);
            float ts = 190 * lerp(0.86f, 1, ease::outBack(ta));
            text("Integration", 0, TY - 70, ts, INK, ta);
            float ua = ease::outCubic(prog(t, 1.9f, 2.7f));
            line(-260 * ua, TY - 190, 260 * ua, TY - 190, 8, GOLD, ua);
            text("the area under a curve, one thin slice at a time", 0, TY - 260, 48, MUTED, prog(t, 2.4f, 3.2f));
        }

        void SliceRange(int from, int to, float u, float xlo, float xhi, float alpha, const std::function<float(int)> &grow)
        {
            int nTo = STAGES[to - 1], nFrom = STAGES[from - 1];
            float dxTo = (B - A) / nTo, dxFrom = (B - A) / nFrom;
            int i0 = std::max(1, static_cast<int>(std::floor((xlo - A) / dxTo + 1e-6f)) + 1);
            int i1 = std::min(nTo, static_cast<int>(std::ceil((xhi - A) / dxTo - 1e-6f)));
            if (i1 < i0)
                return;
            bool split = from != to && u < 1;
            float outlineA = nTo <= 32 ? 1 : (nTo == 64 ? 1 - u : 0);
            float gapK = nTo <= 32 ? 0 : (nTo == 64 ? u : 1);
            float fillA = nTo <= 8 ? 0.5f : (nTo <= 32 ? 0.58f : (nTo == 64 ? lerp(0.58f, 0.82f, u) : 0.82f));
            float gapRatio = nTo <= 256 ? 0.14f : 0.08f;
            for (int i = i0; i <= i1; ++i)
            {
                float cl = A + (i - 1) * dxTo;
                float h = f(cl);
                if (split)
                    h = lerp(f(A + (std::ceil(i / 2.0f) - 1) * dxFrom), h, u);
                if (grow)
                    h *= grow(i);
                float x0 = px(cl), x1 = px(cl + dxTo), top = py(h), hh = top - OY;
                if (hh <= 0.5f)
                    continue;
                float tt = (cl + dxTo * 0.5f - A) / (B - A), w = x1 - x0;
                rect((x0 + x1) * 0.5f, OY + hh * 0.5f, w - w * gapRatio * gapK, hh, mix(CORAL, GOLD, tt), alpha * fillA);
                if (outlineA > 0.01f)
                    pline({{x0, OY}, {x0, top}, {x1, top}, {x1, OY}}, 3 * z, mix(mix(CORAL, GOLD, tt), INK, 0.35f), alpha * outlineA * 0.95f, false);
            }
        }

        void DrawSlices(int from, int to, float u, float alpha, const std::function<float(int)> &grow)
        {
            if (STAGES[to - 1] <= 256)
                SliceRange(from, to, u, A, B, alpha, grow);
            else
            {
                SliceRange(7, 7, 1, WIN_B, B, alpha, nullptr);
                SliceRange(from, to, u, A, WIN_B, alpha, nullptr);
            }
        }

        static std::vector<float> SliceAreas()
        {
            std::vector<float> out;
            float dx = (B - A) / 4;
            for (int i = 1; i <= 4; ++i)
                out.push_back(f(A + (i - 1) * dx) * dx);
            return out;
        }

        void DrawGraph()
        {
            if (t < S("pan").a || t > S("panUp").b)
                return;
            Stage st = StageAt(t);
            const float w3 = 3.5f * z;

            SetZ(2);
            float axU = seg(t, "axes");
            if (t >= S("axes").a)
            {
                float e = ease::outCubic(prog(axU, 0, 0.7f));
                float x0 = px(-0.4f), x1 = px(5.55f);
                line(x0, OY, lerp(x0, x1, e), OY, w3, INK, 0.8f);
                float ey = ease::outCubic(prog(axU, 0.1f, 0.8f));
                float y0 = OY - 20, y1 = py(3.95f);
                line(OX, y0, OX, lerp(y0, y1, ey), w3, INK, 0.8f);
                if (e > 0.98f)
                {
                    line(x1, OY, x1 - 18 * z * 2, OY + 10 * z * 2, w3, INK, 0.8f);
                    line(x1, OY, x1 - 18 * z * 2, OY - 10 * z * 2, w3, INK, 0.8f);
                }
                if (ey > 0.98f)
                {
                    line(OX, y1, OX - 10 * z * 2, y1 - 18 * z * 2, w3, INK, 0.8f);
                    line(OX, y1, OX + 10 * z * 2, y1 - 18 * z * 2, w3, INK, 0.8f);
                }
                float ta = prog(axU, 0.6f, 1);
                if (ta > 0)
                {
                    for (int i = 0; i <= 5; ++i)
                    {
                        line(px(i), OY - 9, px(i), OY + 9, w3 * 0.7f, INK, ta * 0.7f);
                        text(std::to_string(i), px(i), OY - 40, 28, MUTED, ta);
                    }
                    for (int i = 1; i <= 3; ++i)
                    {
                        line(OX - 9, py(i), OX + 9, py(i), w3 * 0.7f, INK, ta * 0.7f);
                        text(std::to_string(i), OX - 36, py(i), 28, MUTED, ta);
                    }
                    text("x", px(5.55f) + 32, OY, 36, MUTED, ta);
                    text("y", OX, py(3.95f) + 38, 36, MUTED, ta);
                }
            }

            float curveU = seg(t, "curve");
            float xr = A + (B - A) * ease::inOutSine(curveU);
            if (t >= S("curve").a)
            {
                const Pts pts = curveU >= 1 ? CURVE_FULL : curveFlat(xr, 361);
                SetZ(3);
                pline(pts, 22 * z, CYAN, 0.1f, false);
                pline(pts, 7 * z, CYAN, 1, false);
                pline(pts, 2.4f * z, INK, 0.55f, false);
                float la = prog(t, S("curve").b - 1.2f, S("curve").b - 0.4f);
                text("y = f(x)", px(1.95f) + 70, py(3.72f), 44, CYAN, la);
            }

            SetZ(0);
            float areaA = 0.26f;
            areaA = lerp(areaA, 0.1f, prog(t, S("big").a, S("big").a + 1.2f));
            areaA = lerp(areaA, 0.4f, ease::inOutSine(prog(t, S("smooth").a, S("smooth").a + 1.6f)));
            float sweep = ease::inOutCubic(prog(t, S("area").a + 1.4f, S("area").a + 3.4f));
            if (t >= S("area").a + 1.4f)
                poly(areaPoly(A + (B - A) * sweep), TEAL, areaA);

            if (t >= S("area").a)
            {
                SetZ(4);
                float bu = ease::outCubic(prog(t, S("area").a, S("area").a + 1.2f));
                for (float v : {A, B})
                    dashed(px(v), OY, px(v), py(f(v)), 3.2f * z, 14 * z, 10 * z, GOLD, 0.9f, bu);
                float ba = ease::outBack(prog(t, S("area").a + 0.8f, S("area").a + 1.5f));
                text("a", px(A) - 6, OY - 104, 54 * clamp01(ba), GOLD, ba);
                text("b", px(B) + 6, OY - 104, 54 * clamp01(ba), GOLD, ba);
                float qa = prog(t, S("area").a + 2.4f, S("area").a + 3.2f) * (1 - prog(t, S("big").a, S("big").a + 0.6f));
                if (qa > 0)
                {
                    float bob = std::sin(t * 2.4f) * 10;
                    text("?", px(2.5f), py(1.0f) + bob, 210, INK, qa * (0.55f + 0.25f * std::sin(t * 2.4f)));
                }
            }

            if (t >= S("big").a && t < S("smooth").b)
            {
                SetZ(1);
                float alpha = 1 - ease::inOutSine(prog(t, S("smooth").a + 0.4f, S("smooth").a + 2.2f));
                float starts[4];
                for (int i = 0; i < 4; ++i)
                    starts[i] = S("big").a + 0.4f + i * 0.65f;
                std::function<float(int)> grow;
                if (st.to == 1)
                    grow = [&](int i)
                    { return ease::outBack(prog(t, starts[i - 1] + 0.3f, starts[i - 1] + 1.05f)); };
                DrawSlices(st.from, st.to, st.u, alpha, grow);

                float dotA = st.to == 1 ? 1 : (st.from == 1 && st.to == 2 ? 1 - st.u : 0);
                if (dotA > 0.01f)
                {
                    SetZ(5);
                    for (int i = 1; i <= 4; ++i)
                    {
                        float xl = A + (i - 1) * (B - A) / 4;
                        float d = prog(t, starts[i - 1], starts[i - 1] + 0.4f);
                        if (d <= 0)
                            continue;
                        dashed(px(xl), OY, px(xl), py(f(xl)), 2.6f * z, 10 * z, 8 * z, INK, 0.7f * dotA, ease::outCubic(d));
                        float pop = ease::outBack(d);
                        circle(px(xl), py(f(xl)), 12 * z * pop, INK, dotA);
                        ring(px(xl), py(f(xl)), 17 * z * pop, 3 * z, CORAL, dotA);
                    }
                }
            }

            SetZ(6);
            DrawSliceNotes();
            SetZ(7);
            DrawPen(curveU, xr);
            DrawReadout(st);
        }

        void DrawSliceNotes()
        {
            if (t < S("bigIn").a || t > S("split1").a + 0.5f)
                return;
            std::vector<float> areas = SliceAreas();
            float dx = (B - A) / 4, x0 = px(A), x1 = px(A + dx), top = py(f(A));
            float fade = 1 - prog(t, S("bigOut").a, S("bigOut").a + 0.7f);
            float w3 = 4 * z;

            float pw = ease::outCubic(prog(t, S("bigIn").a + 0.7f, S("bigIn").a + 1.4f)) * fade;
            if (pw > 0)
            {
                float yb = OY - 58;
                line(x0, yb, x1, yb, w3, TEAL, pw);
                line(x0, yb + 12, x0, yb - 12, w3, TEAL, pw);
                line(x1, yb + 12, x1, yb - 12, w3, TEAL, pw);
                float la = ease::outBack(prog(t, S("bigIn").a + 1.0f, S("bigIn").a + 1.7f)) * fade;
                deltaX((x0 + x1) / 2, yb - 62, 62 * clamp01(la), TEAL, la);
                text("width", (x0 + x1) / 2, yb - 128, 30, MUTED, la * 0.9f);
            }

            float ph = ease::outCubic(prog(t, S("bigNote").a, S("bigNote").a + 0.7f)) * fade;
            if (ph > 0)
            {
                float xb = OX - 62, yt = lerp(OY, top, ph);
                line(xb, OY, xb, yt, w3, CORAL, ph);
                line(xb - 12, OY, xb + 12, OY, w3, CORAL, ph);
                line(xb - 12, yt, xb + 12, yt, w3, CORAL, ph);
                float la = ease::outBack(prog(t, S("bigNote").a + 0.4f, S("bigNote").a + 1.1f)) * fade;
                text("f(x)", xb - 22, (OY + top) / 2 + 20, 62 * clamp01(la), CORAL, la, TextAlign::Right);
                text("height", xb - 22, (OY + top) / 2 - 36, 30, MUTED, la * 0.9f, TextAlign::Right);
            }

            float pa = prog(t, S("bigNote").a + 1.6f, S("bigNote").a + 2.4f) * fade;
            if (pa > 0)
            {
                float mx = (x0 + x1) / 2, my = OY + (top - OY) * 0.5f + 30, s = 44, lx = mx - 4.36f * s / 2;
                text("f(x)", lx, my, s, CORAL, pa, TextAlign::Left);
                text("x", lx + 2.14f * s, my, s, INK, pa, TextAlign::Left);
                deltaX(lx + 3.64f * s, my, s, TEAL, pa);
                float pe = prog(t, S("bigNote").a + 2.5f, S("bigNote").a + 3.2f) * fade;
                text(Fmt("= %.2f", areas[0]), mx, my - 64, 52, INK, pe);
            }

            if (prog(t, S("bigOut").a, S("bigOut").a + 0.3f) > 0)
            {
                for (int i = 1; i <= 4; ++i)
                {
                    float xl = A + (i - 1) * dx;
                    float a = prog(t, S("bigSum").a + 0.1f + (i - 1) * 0.35f, S("bigSum").a + 0.5f + (i - 1) * 0.35f);
                    float gone = prog(t, S("split1").a, S("split1").a + 0.4f);
                    float h = py(f(xl)) - OY;
                    text(Fmt("%.2f", areas[i - 1]), px(xl + dx / 2), OY + h * 0.5f, 50, INK, a * (1 - gone));
                }
            }
        }

        void DrawPen(float curveU, float xr)
        {
            (void)curveU;
            float pa = (t >= S("curve").a && t < S("area").a + 0.4f) ? (1 - prog(t, S("curve").b, S("curve").b + 0.7f)) : 0;
            if (pa <= 0)
                return;
            float x = px(xr), y = py(f(xr));
            dashed(x, OY, x, y, 2.6f * z, 9 * z, 8 * z, INK, 0.55f * pa);
            dashed(OX, y, x, y, 2.6f * z, 9 * z, 8 * z, INK, 0.4f * pa);
            float pulse = 0.5f + 0.5f * std::sin(t * 8);
            circle(x, y, 26 * z, CYAN, 0.16f * pa * (0.6f + 0.4f * pulse));
            circle(x, y, 13 * z, INK, pa);
            ring(x, y, 21 * z, 3 * z, CYAN, pa);
            text(Fmt("x = %.2f", xr), x + 16, OY + 34, 32, INK, pa, TextAlign::Left);
            text(Fmt("f(x) = %.2f", f(xr)), x + 20, y + 54, 34, CYAN, pa);
        }

        void DrawReadout(const Stage &st)
        {
            float a = prog(t, S("big").a + 1.0f, S("big").a + 1.8f) * (1 - prog(t, S("panUp").a, S("panUp").a + 0.6f));
            if (a <= 0)
                return;
            const float rx = 470, ry = 262;
            int n;
            float sum;
            if (t < S("split1").a)
            {
                std::vector<float> areas = SliceAreas();
                sum = 0;
                for (int i = 1; i <= 4; ++i)
                {
                    float s0 = S("big").a + 0.4f + (i - 1) * 0.65f;
                    sum += areas[i - 1] * ease::outCubic(prog(t, s0 + 0.3f, s0 + 1.0f));
                }
                n = 4;
            }
            else
            {
                n = (st.u > 0 && st.from != st.to) ? STAGES[st.to - 1] : STAGES[st.from - 1];
                sum = lerp(SUMS[st.from - 1], SUMS[st.to - 1], st.u);
            }
            float lim = ease::inOutSine(prog(t, S("smooth").a + 0.6f, S("smooth").a + 2.4f));
            sum = lerp(sum, TRUE_AREA, lim);
            float inf = prog(t, S("smooth").a + 0.6f, S("smooth").a + 1.4f);
            SetZ(8);
            rrect(rx, ry, 612, 210, 30, VIOLET, a * 0.5f);
            rrect(rx, ry, 606, 204, 27, PANEL, a * 0.96f);
            float xl = rx - 264, xr = rx + 264;
            text("slices", xl, ry + 56, 32, MUTED, a, TextAlign::Left);
            text("sum of areas", xl, ry, 32, MUTED, a, TextAlign::Left);
            text("error", xl, ry - 56, 32, MUTED, a, TextAlign::Left);
            text(std::to_string(n), xr, ry + 56, 46, INK, a * (1 - inf), TextAlign::Right);
            if (inf > 0)
                infinity(xr - 34, ry + 56, 26, INK, a * inf, 5);
            text(Fmt("%.4f", sum), xr, ry, 46, GOLD, a, TextAlign::Right);
            text(Fmt("%.4f", std::fabs(TRUE_AREA - sum)), xr, ry - 56, 46, CORAL, a, TextAlign::Right);
        }

        void DrawBoard()
        {
            if (t < S("smooth").a + 1.0f)
                return;
            const float SZ = 112, row = BY + 170;
            const Span &fs = S("fSum"), &fe = S("fExplain"), &ft = S("fThin"), &fm = S("fMorph"), &fr = S("fResult"), &fo = S("outro");
            SetZ(10);

            float mAppr = ease::inOutCubic(prog(t, fm.a, fm.a + 1.0f));
            float mGlyph = ease::inOutCubic(prog(t, fm.a + 0.4f, fm.a + 2.0f));
            float mLimit = ease::inOutCubic(prog(t, fm.a + 1.2f, fm.a + 2.2f));
            float mI = ease::inOutCubic(prog(t, fm.a + 0.8f, fm.a + 1.6f));
            float mD = ease::inOutCubic(prog(t, fm.a + 1.4f, fm.a + 2.4f));

            float aa = clamp01(ease::outBack(prog(t, fs.a + 0.1f, fs.a + 0.8f)));
            text("Area", -475, row - (1 - aa) * 40, SZ, INK, aa, TextAlign::Left);

            float ap = prog(t, fs.a + 0.5f, fs.a + 1.0f);
            approx(-155, row, SZ * 0.8f, mAppr, mix(MUTED, INK, mAppr), ap);

            float draw = ease::inOutSine(prog(t, fs.a + 0.8f, fs.a + 2.0f));
            Rgb gcol = mix(VIOLET, GOLD, mGlyph);
            Pts pts = glyphPoints(-10, row + 4, SZ * 1.9f, mGlyph, draw);
            pline(pts, 30 * 0.6f, gcol, 0.14f, false);
            pline(pts, 13, gcol, 1, false);

            float la = prog(t, fs.a + 1.8f, fs.a + 2.4f);
            text("n", lerp(-10, 72, mLimit), lerp(row + 146, row + 120, mLimit), 50, MUTED, la * (1 - mLimit));
            text("i = 1", lerp(-10, -40, mLimit), lerp(row - 146, row - 130, mLimit), 44, MUTED, la * (1 - mLimit));
            text("b", lerp(-10, 72, mLimit), lerp(row + 146, row + 120, mLimit), 56, GOLD, la * mLimit);
            text("a", lerp(-10, -40, mLimit), lerp(row - 146, row - 130, mLimit), 56, GOLD, la * mLimit);

            float pf = ease::outCubic(prog(t, fs.a + 2.0f, fs.a + 2.7f));
            float fy = row - (1 - pf) * 30, fx = 93, iw = 0.145f * SZ;
            text("f(x", fx, fy, SZ, CORAL, pf, TextAlign::Left);
            text("i", fx + 1.27f * SZ, fy - 0.22f * SZ, SZ * 0.5f, CORAL, pf * (1 - mI), TextAlign::Left);
            text(")", fx + 1.27f * SZ + iw * (1 - mI), fy, SZ, CORAL, pf, TextAlign::Left);

            float pd = ease::outCubic(prog(t, fs.a + 2.6f, fs.a + 3.3f));
            float dy = row - (1 - pd) * 30, left = 324 - 16 * mI;
            deltaX(left + 0.72f * SZ * (1 - 0.3f * mD), dy, SZ * (1 - 0.3f * mD), TEAL, pd * (1 - mD));
            text("dx", left, dy, SZ, TEAL, pd * mD, TextAlign::Left);

            float hiC = std::sin(PI * prog(t, fe.a + 0.9f, fe.a + 1.9f));
            float hiT = std::sin(PI * prog(t, fe.a + 1.9f, fe.a + 2.9f));
            if (hiC > 0)
                rrect(fx + 105, row, 270, SZ * 1.05f, 26, CORAL, 0.2f * hiC);
            if (hiT > 0)
                rrect(left + 85, row, 215, SZ * 1.05f, 26, TEAL, 0.2f * hiT);

            DrawBoardPicture();

            float n1 = prog(t, ft.a + 0.2f, ft.a + 0.8f) * (1 - prog(t, fm.a, fm.a + 0.8f));
            float n2 = prog(t, ft.a + 1.2f, ft.a + 1.8f) * (1 - prog(t, fm.a, fm.a + 0.8f));
            if (n1 > 0)
            {
                deltaX(190, BY + 20, 64, TEAL, n1);
                arrow(262, BY + 20, 352, BY + 20, 5, INK, n1);
                text("0", 395, BY + 20, 64, TEAL, n1);
            }
            if (n2 > 0)
            {
                text("n", 190, BY - 86, 64, GOLD, n2);
                arrow(235, BY - 86, 325, BY - 86, 5, INK, n2);
                infinity(398, BY - 86, 40, GOLD, n2, 7);
            }

            float ra = prog(t, fr.a + 0.2f, fr.a + 0.9f);
            if (ra > 0)
            {
                float num = TRUE_AREA * ease::outCubic(prog(t, fr.a + 0.8f, fr.a + 3.0f));
                float glow = 0.5f + 0.5f * std::sin(t * 3);
                rrect(0, BY - 105, 700, 200, 44, GOLD, ra * (0.18f + 0.12f * glow));
                rrect(0, BY - 105, 690, 190, 40, PANEL, ra * 0.97f);
                text("from a = 0 to b = 5", 0, BY - 40, 36, MUTED, ra);
                text(Fmt("%.3f", num), 0, BY - 128, 130, GOLD, ra);
            }

            float oa = prog(t, fo.a + 0.2f, fo.a + 1.0f);
            if (oa > 0)
            {
                text("Integration is adding up", 0, BY - 300, 58, INK, oa);
                text("infinitely many, infinitely thin slices", 0, BY - 368, 58, GOLD, prog(t, fo.a + 0.7f, fo.a + 1.5f));
            }
        }

        void DrawBoardPicture()
        {
            const Span &fe = S("fExplain"), &ft = S("fThin"), &fm = S("fMorph");
            float pa = prog(t, fe.a, fe.a + 0.8f) * (1 - prog(t, fm.a + 0.2f, fm.a + 1.0f));
            if (pa <= 0)
                return;
            const float base = BY - 300, left = -640, right = -140;
            auto yc = [&](float x)
            { return base + 70 + 62 * std::sin((x - left) / (right - left) * 2.6f); };
            Pts curve;
            for (int i = 0; i <= 40; ++i)
            {
                float x = left + (right - left) * i / 40;
                curve.push_back({x, yc(x)});
            }
            float wide = lerp(170, 9, ease::inOutCubic(prog(t, ft.a, ft.a + 2.6f)));
            float x0 = left + 30;
            float gr = ease::outCubic(prog(t, fe.a + 0.3f, fe.a + 1.0f));

            line(left - 20, base, right + 20, base, 3 * z, INK, pa * 0.6f);
            float tile = prog(t, fe.a + 2.5f, fe.a + 3.1f);
            if (tile > 0)
            {
                int k = static_cast<int>(std::floor((right - (x0 + wide)) / wide));
                for (int i = 1; i <= std::min(k, 60); ++i)
                {
                    float xl = x0 + i * wide, h = yc(xl) - base, gap = std::min(2.5f, wide * 0.15f);
                    rect(xl + wide / 2, base + h / 2, wide - gap, h, mix(CORAL, GOLD, static_cast<float>(i) / std::max(k, 1)), 0.38f * pa * tile);
                }
            }
            float h0 = (yc(x0) - base) * gr;
            rect(x0 + wide / 2, base + h0 / 2, wide - std::min(2.5f, wide * 0.15f), h0, CORAL, 0.8f * pa);
            pline(curve, 5 * z, CYAN, pa, false);

            float hb = ease::outCubic(prog(t, fe.a + 0.9f, fe.a + 1.5f)) * pa;
            if (hb > 0)
            {
                float xb = x0 - 20, top = base + (yc(x0) - base);
                line(xb, base, xb, top, 3.5f * z, CORAL, hb);
                line(xb - 9, base, xb + 9, base, 3.5f * z, CORAL, hb);
                line(xb - 9, top, xb + 9, top, 3.5f * z, CORAL, hb);
                text("f(x)", xb - 18, (base + top) / 2, 44, CORAL, hb, TextAlign::Right);
            }
            float wb = ease::outCubic(prog(t, fe.a + 1.8f, fe.a + 2.4f)) * pa;
            if (wb > 0)
            {
                float yb = base - 34;
                line(x0, yb, x0 + wide, yb, 3.5f * z, TEAL, wb);
                line(x0, yb + 9, x0, yb - 9, 3.5f * z, TEAL, wb);
                line(x0 + wide, yb + 9, x0 + wide, yb - 9, 3.5f * z, TEAL, wb);
                float sw = prog(wide, 60, 30);
                deltaX(x0 + wide / 2, yb - 52, 50, TEAL, wb * (1 - sw));
                text("dx", x0 + wide / 2, yb - 52, 50, TEAL, wb * sw);
            }
        }

        void DrawHud()
        {
            SetZ(60);
            for (const Caption &c : CAPS)
            {
                float a = S(c.from).a + 0.25f, b = S(c.to).b - 0.25f;
                float vis = prog(t, a, a + 0.45f) * (1 - prog(t, b - 0.45f, b));
                if (vis <= 0.01f)
                    continue;
                float w = std::max(tw(c.title, 22), tw(c.body, 36)) * z + 120 * z;
                float y = cy - 540 * z + 104 * z - (1 - vis) * 16 * z;
                rrect(cx, y, w, 104 * z, 26 * z, BG, vis * 0.82f);
                text(c.title, cx, y + 24 * z, 22 * z, GOLD, vis);
                text(c.body, cx, y - 14 * z, 36 * z, INK, vis);
            }
            float bw = 1500 * z, by = cy - 540 * z + 24 * z, p = t / TOTAL;
            line(cx - bw / 2, by, cx + bw / 2, by, 4 * z, INK, 0.12f);
            line(cx - bw / 2, by, cx - bw / 2 + bw * p, by, 4 * z, GOLD, 0.9f);
            circle(cx - bw / 2 + bw * p, by, 7 * z, GOLD, 1);
            text("HOW INTEGRATION WORKS", cx, cy + 540 * z - 38 * z, 22 * z, MUTED, 0.55f);

            if (t >= S("diveIn").a && t < S("diveOut").b)
            {
                float a = prog(t, S("diveIn").a + 0.8f, S("diveIn").a + 1.4f) * (1 - prog(t, S("diveOut").a, S("diveOut").a + 0.5f));
                Stage st = StageAt(t);
                int n = (st.u > 0 && st.from != st.to) ? STAGES[st.to - 1] : STAGES[st.from - 1];
                float y = cy + 540 * z - 110 * z;
                rrect(cx, y, 330 * z, 80 * z, 22 * z, BG, a * 0.85f);
                text("slices  n = " + std::to_string(n), cx, y, 40 * z, GOLD, a);
            }

            float fade = std::max(1 - prog(t, 0, 0.9f), prog(t, S("fade").a, S("fade").b));
            if (fade > 0.002f)
            {
                SetZ(100);
                rect(cx, cy, 9000 * z, 9000 * z, BG, fade);
            }
        }

        void Render(float aspect)
        {
            ApplyCamera();
            Renderer::SetClearColor(BG.r / 255, BG.g / 255, BG.b / 255, 1);
            Renderer::Clear();
            const float hh = 540 * z, hw = hh * aspect;
            Renderer::BeginScene(glm::mat4(1.0f), glm::ortho(cx - hw, cx + hw, cy - hh, cy + hh, -1000.0f, 1000.0f));
            DrawBackdrop();
            DrawTitle();
            DrawGraph();
            DrawBoard();
            DrawHud();
            Renderer::EndScene();
        }
    };

    struct Quality
    {
        const char *name;
        int width, height, fps, samples, crf;
        const char *preset;
    };

    const Quality QUALITIES[] = {
        {"draft", 960, 540, 30, 2, 26, "veryfast"},
        {"hd", 1280, 720, 30, 4, 18, "medium"},
        {"fhd", 1920, 1080, 60, 4, 17, "medium"},
        {"4k", 3840, 2160, 30, 4, 16, "fast"},
    };

    const Quality *FindQuality(const std::string &name)
    {
        for (const Quality &q : QUALITIES)
            if (name == q.name)
                return &q;
        return nullptr;
    }

    double Ms(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b)
    {
        return std::chrono::duration<double, std::milli>(b - a).count();
    }
}

int main(int argc, char **argv)
{
    std::string mode = "video", out = "integration.mp4";
    Quality q = *FindQuality("fhd");
    int benchFrames = 300;
    std::vector<float> times;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--quality" && i + 1 < argc)
        {
            const Quality *found = FindQuality(argv[++i]);
            if (!found)
            {
                std::fprintf(stderr, "unknown quality '%s' (draft, hd, fhd, 4k)\n", argv[i]);
                return 1;
            }
            q = *found;
        }
        else if (arg == "--fps" && i + 1 < argc)
            q.fps = std::max(1, std::atoi(argv[++i]));
        else if (arg == "--frames" && i + 1 < argc)
            benchFrames = std::max(1, std::atoi(argv[++i]));
        else if (arg == "stills" || arg == "bench")
            mode = arg;
        else if (mode == "stills")
            times.push_back(static_cast<float>(std::atof(arg.c_str())));
        else
            out = arg;
    }

    SIMPEngine::Log::Init();
    SIMPEngine::VFS::Mount("engine", SIMP_ASSETS_DIR);
    SIMPEngine::Window window;
    if (!window.Init("IntegrationVideo", 320, 180, SDL_WINDOW_HIDDEN))
        return 1;
    Renderer::Init(std::make_unique<SIMPEngine::GLRenderingAPI>(), 320, 180);

    BuildTimeline();
    BuildShapes();
    BuildPath();

    const float dt = 1.0f / static_cast<float>(q.fps);
    const float aspect = static_cast<float>(q.width) / static_cast<float>(q.height);
    SIMPEngine::Framebuffer fb(q.width, q.height, false, q.samples);
    std::vector<uint8_t> pixels;
    Integration video;

    auto frame = [&](float t)
    {
        video.t = t;
        Renderer::BeginRenderTarget(fb);
        video.Render(aspect);
        fb.ReadPixels(pixels);
        Renderer::EndRenderTarget();
    };

    int status = 0;
    if (mode == "stills")
    {
        for (size_t i = 0; i < times.size(); ++i)
        {
            frame(std::round(times[i] * q.fps) * dt);
            char name[64];
            std::snprintf(name, sizeof(name), "cpp_%02zu.png", i);
            SIMPEngine::ImageWriter::WritePng(name, q.width, q.height, pixels);
        }
    }
    else if (mode == "bench")
    {
        double draw = 0, read = 0;
        const int total = static_cast<int>(TOTAL * q.fps);
        for (int i = 0; i < benchFrames; ++i)
        {
            video.t = std::fmod((static_cast<float>(i) * total / benchFrames + 1) * dt, TOTAL);
            auto t0 = std::chrono::steady_clock::now();
            Renderer::BeginRenderTarget(fb);
            video.Render(aspect);
            auto t1 = std::chrono::steady_clock::now();
            fb.ReadPixels(pixels);
            Renderer::EndRenderTarget();
            auto t2 = std::chrono::steady_clock::now();
            draw += Ms(t0, t1);
            read += Ms(t1, t2);
        }
        std::printf("C++ %s %dx%d: scene %.2f ms/frame, gpu+readback %.2f ms/frame (%d frames)\n", q.name, q.width, q.height, draw / benchFrames, read / benchFrames, benchFrames);
    }
    else
    {
        char cmd[512];
        std::snprintf(cmd, sizeof(cmd), "ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgba -s %dx%d -r %d -i - -c:v libx264 -preset %s -crf %d -pix_fmt yuv420p -movflags +faststart \"%s\"",
                      q.width, q.height, q.fps, q.preset, q.crf, out.c_str());
        FILE *pipe = popen(cmd, "w");
        if (!pipe)
            return 3;
        const int frames = static_cast<int>(TOTAL * q.fps);
        for (int i = 0; i < frames; ++i)
        {
            float t = (static_cast<float>(i) + 1) * dt;
            frame(t >= TOTAL ? 0.0f : t);
            std::fwrite(pixels.data(), 1, pixels.size(), pipe);
            if (i % (q.fps * 10) == 0)
                std::fprintf(stderr, "frame %d/%d\n", i, frames);
        }
        status = pclose(pipe) == 0 ? 0 : 4;
    }

    SIMPEngine::Font::ClearCache();
    Renderer::Shutdown();
    window.ShutDown();
    return status;
}
