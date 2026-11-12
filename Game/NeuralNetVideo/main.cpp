#include <Engine/Core/Log.h>
#include <Engine/Core/Window.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/GLRenderingAPI.h>
#include <Engine/Rendering/ImageWriter.h>
#include <Engine/Rendering/Renderer.h>

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "NeuralNet.h"

using SIMPEngine::Framebuffer;
using SIMPEngine::Renderer;
using SIMPEngine::TextAlign;
using Col = SDL_Color;

namespace
{
    constexpr int W = 1280, H = 720, FPS = 30;
    constexpr float DUR = 90.0f;
    constexpr int SNAP_EVERY = 25, EPOCHS = 6000;

    std::shared_ptr<SIMPEngine::Font> g_font;
    std::vector<Dot> g_dots, g_test;
    std::vector<Net> g_snaps;
    std::vector<float> g_loss;
    int g_sample = 0;

    float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
    float Seg(float t, float a, float b) { return Clamp01((t - a) / (b - a)); }
    float EaseIO(float x) { return x < 0.5f ? 4 * x * x * x : 1 - std::pow(-2 * x + 2, 3.0f) / 2; }
    float Back(float x) { const float c = 1.9f; float k = x - 1; return 1 + (c + 1) * k * k * k + c * k * k; }
    float Win(float t, float a, float b, float f = 0.4f) { return std::min(Seg(t, a, a + f), 1 - Seg(t, b - f, b)); }
    float Lerp(float a, float b, float t) { return a + (b - a) * t; }

    Col Rgb(int r, int g, int b, float a = 1.0f) { return {(Uint8)r, (Uint8)g, (Uint8)b, (Uint8)(Clamp01(a) * 255)}; }
    Col A(Col c, float a) { c.a = (Uint8)(c.a * Clamp01(a)); return c; }
    Col Mix(Col a, Col b, float t)
    {
        t = Clamp01(t);
        return {(Uint8)Lerp(a.r, b.r, t), (Uint8)Lerp(a.g, b.g, t), (Uint8)Lerp(a.b, b.b, t), (Uint8)Lerp(a.a, b.a, t)};
    }

    const Col BG = Rgb(15, 18, 27), PANEL = Rgb(25, 30, 45), EDGE = Rgb(62, 71, 100), MUTED = Rgb(150, 160, 188), WHITE = Rgb(238, 241, 250);
    const Col BLUE = Rgb(86, 166, 255), ORANGE = Rgb(255, 152, 66), TEAL = Rgb(92, 204, 172), PINK = Rgb(238, 98, 140), AMBER = Rgb(255, 205, 90);

    std::string Fmt(const char *f, ...)
    {
        char buf[256];
        va_list ap;
        va_start(ap, f);
        std::vsnprintf(buf, sizeof(buf), f, ap);
        va_end(ap);
        return buf;
    }

    void Text(const std::string &s, float x, float yc, float size, Col c, TextAlign al = TextAlign::Center, float z = 0.6f)
    {
        if (c.a == 0)
            return;
        Renderer::DrawText(s, x, yc + size * 0.6f, size, c, al, 0.0f, z, g_font.get());
    }

    void Line(float x1, float y1, float x2, float y2, float w, Col c, float z = 0.3f) { if (c.a) Renderer::DrawLine(x1, y1, x2, y2, w, c, z); }

    Net Blend(const Net &a, const Net &b, float f)
    {
        Net n{};
        for (int i = 0; i < 4; ++i)
        {
            n.W1[i][0] = Lerp(a.W1[i][0], b.W1[i][0], f);
            n.W1[i][1] = Lerp(a.W1[i][1], b.W1[i][1], f);
            n.b1[i] = Lerp(a.b1[i], b.b1[i], f);
            n.W2[i] = Lerp(a.W2[i], b.W2[i], f);
        }
        n.b2 = Lerp(a.b2, b.b2, f);
        return n;
    }

    float IdxAt(float t)
    {
        const float last = static_cast<float>(g_snaps.size() - 1);
        return last * std::pow(Seg(t, 58.0f, 76.0f), 3.2f);
    }

    Net NetAt(float idx)
    {
        int i0 = std::min(static_cast<int>(idx), static_cast<int>(g_snaps.size()) - 1);
        int i1 = std::min(i0 + 1, static_cast<int>(g_snaps.size()) - 1);
        return Blend(g_snaps[i0], g_snaps[i1], idx - static_cast<float>(i0));
    }

    struct Panel { float cx, cy, size; };
    glm::vec2 ToPanel(const Panel &p, float x, float y) { return {p.cx + x * p.size * 0.475f, p.cy + y * p.size * 0.475f}; }

    void DrawPanel(const Panel &p, float alpha, const Net *map, float mapAlpha, float dotProgress)
    {
        if (alpha <= 0.001f)
            return;
        Renderer::DrawRoundedRect(p.cx, p.cy, p.size / 2 + 5, p.size / 2 + 5, 16, A(EDGE, alpha), 1.0f, 0.00f);
        Renderer::DrawRoundedRect(p.cx, p.cy, p.size / 2, p.size / 2, 12, A(PANEL, alpha), 1.0f, 0.01f);
        if (map && mapAlpha > 0.001f)
        {
            const int N = 36;
            const float cell = p.size * 0.95f / N;
            for (int j = 0; j < N; ++j)
                for (int i = 0; i < N; ++i)
                {
                    float x = (i + 0.5f) / N * 2 - 1, y = (j + 0.5f) / N * 2 - 1;
                    float q = Eval(*map, x, y);
                    float conf = std::fabs(2 * q - 1);
                    glm::vec2 c = ToPanel(p, x, y);
                    Renderer::DrawQuad(c.x, c.y, cell + 0.6f, cell + 0.6f, 0, A(Mix(ORANGE, BLUE, q), (0.10f + 0.42f * conf) * mapAlpha * alpha), true, 0.05f);
                }
        }
        const float k = p.size / 360.0f;
        const int n = static_cast<int>(g_dots.size());
        for (int i = 0; i < n; ++i)
        {
            float start = static_cast<float>(i) / n;
            float pop = Seg(dotProgress * 1.2f, start, start + 0.2f);
            if (pop <= 0)
                continue;
            glm::vec2 c = ToPanel(p, g_dots[i].x, g_dots[i].y);
            float r = 5.6f * k * Back(pop);
            Col col = g_dots[i].label > 0.5f ? BLUE : ORANGE;
            Renderer::DrawCircle(c.x, c.y, r + 1.6f * k, A(Rgb(10, 12, 20), alpha), 1.0f, 0.10f);
            Renderer::DrawCircle(c.x, c.y, r, A(col, alpha), 1.0f, 0.11f);
        }
    }

    struct NetLayout { float cx, cy, w, h; };
    glm::vec2 NodePos(const NetLayout &L, int layer, int i)
    {
        int cnt = layer == 0 ? 2 : (layer == 1 ? 4 : 1);
        float sp = L.h / 4.0f;
        return {L.cx + (layer - 1) * L.w / 2, L.cy + ((cnt - 1) * 0.5f - i) * sp};
    }

    void DrawNet(const NetLayout &L, const Net &net, float alpha, float nodeReveal, float edgeReveal, float lit, float sx, float sy)
    {
        if (alpha <= 0.001f)
            return;
        float h[4];
        float p = Eval(net, sx, sy, h);
        const float r = std::min(24.0f, L.w * 0.07f);
        float e1 = Seg(edgeReveal, 0.0f, 0.5f), e2 = Seg(edgeReveal, 0.5f, 1.0f);
        auto edge = [&](glm::vec2 a, glm::vec2 b, float w, float reveal)
        {
            if (reveal <= 0)
                return;
            float m = std::fabs(w);
            Col c = A(w >= 0 ? TEAL : PINK, (0.30f + 0.70f * std::min(m / 1.6f, 1.0f)) * alpha);
            glm::vec2 e = a + (b - a) * EaseIO(reveal);
            Line(a.x, a.y, e.x, e.y, 1.2f + std::min(m, 2.6f) * 1.5f, c, 0.30f);
        };
        for (int i = 0; i < 4; ++i)
        {
            for (int k = 0; k < 2; ++k)
                edge(NodePos(L, 0, k), NodePos(L, 1, i), net.W1[i][k], e1);
            edge(NodePos(L, 1, i), NodePos(L, 2, 0), net.W2[i], e2);
        }
        auto node = [&](glm::vec2 c, float reveal, Col fill, const std::string &label)
        {
            if (reveal <= 0)
                return;
            float rr = r * Back(reveal), a = alpha * Clamp01(reveal * 2);
            Renderer::DrawCircle(c.x, c.y, rr + 2.5f, A(Mix(EDGE, WHITE, 0.45f), a), 1.0f, 0.40f);
            Renderer::DrawCircle(c.x, c.y, rr, A(fill, a), 1.0f, 0.41f);
            if (!label.empty())
                Text(label, c.x, c.y, r * 0.95f, A(Rgb(14, 17, 26), a), TextAlign::Center, 0.6f);
        };
        const Col neutral = Rgb(58, 66, 98);
        float rv0 = Seg(nodeReveal, 0.0f, 0.34f), rv1 = Seg(nodeReveal, 0.34f, 0.67f), rv2 = Seg(nodeReveal, 0.67f, 1.0f);
        node(NodePos(L, 0, 0), rv0, Mix(neutral, WHITE, Seg(lit, 0.0f, 0.3f)), "x");
        node(NodePos(L, 0, 1), rv0, Mix(neutral, WHITE, Seg(lit, 0.0f, 0.3f)), "y");
        for (int i = 0; i < 4; ++i)
            node(NodePos(L, 1, i), rv1, Mix(neutral, Mix(Rgb(70, 80, 140), AMBER, (h[i] + 1) * 0.5f), Seg(lit, 1.0f, 1.4f)), "");
        node(NodePos(L, 2, 0), rv2, Mix(neutral, Mix(ORANGE, BLUE, p), Seg(lit, 2.0f, 2.4f)), "");
        if (lit > 2.4f)
            Text(Fmt("%.2f", p), NodePos(L, 2, 0).x + r + 8, NodePos(L, 2, 0).y, 22, A(WHITE, alpha * Seg(lit, 2.4f, 2.8f)), TextAlign::Left, 0.6f);

        if (lit > 0.0f && lit < 2.0f)
        {
            bool first = lit < 1.0f;
            float t = first ? lit : lit - 1.0f;
            float fade = std::min(Seg(t, 0.0f, 0.1f), 1 - Seg(t, 0.9f, 1.0f)) * alpha;
            for (int i = 0; i < 4; ++i)
                for (int k = 0; k < (first ? 2 : 1); ++k)
                {
                    glm::vec2 a = first ? NodePos(L, 0, k) : NodePos(L, 1, i), b = first ? NodePos(L, 1, i) : NodePos(L, 2, 0);
                    glm::vec2 c = a + (b - a) * EaseIO(t);
                    Renderer::DrawCircle(c.x, c.y, 11, A(AMBER, 0.25f * fade), 1.0f, 0.5f);
                    Renderer::DrawCircle(c.x, c.y, 5.5f, A(WHITE, fade), 1.0f, 0.51f);
                }
        }
        float ly = L.cy - L.h * 0.375f - r - 26;
        Col lab = A(MUTED, alpha * Seg(nodeReveal, 0.8f, 1.0f));
        Text("inputs", NodePos(L, 0, 0).x, ly, 18, lab);
        Text("hidden layer", L.cx, ly, 18, lab);
        Text("output", NodePos(L, 2, 0).x, ly, 18, lab);
    }

    struct Cap { float a, b; const char *text; };
    const Cap CAPS[] = {
        {4.2f, 7.6f, "Every dot has two numbers: where it is (x, y). And it has a color."},
        {7.8f, 11.8f, "Goal: teach a program to tell blue from orange, using only (x, y)."},
        {12.6f, 16.0f, "A neuron is a tiny function. It takes numbers in..."},
        {16.2f, 20.2f, "...multiplies each by a weight, adds a bias..."},
        {20.4f, 24.0f, "...then squashes the result into a number between 0 and 1."},
        {24.2f, 26.9f, "Close to 1 means \"blue\". Close to 0 means \"orange\"."},
        {27.3f, 31.4f, "One neuron can only cut the dots with a straight line. That is not enough here."},
        {31.7f, 37.0f, "So we stack neurons in layers. One layer's outputs feed the next layer."},
        {37.3f, 41.9f, "This network: 2 inputs, 4 hidden neurons, 1 output. Every line is a weight."},
        {42.3f, 47.0f, "At the start the weights are random. The network is just guessing."},
        {47.2f, 51.0f, "We measure how wrong it is with one number: the loss."},
        {51.2f, 53.9f, "Lower loss means better guesses."},
        {54.3f, 57.9f, "Now the training loop: guess, measure the error, nudge the weights."},
        {58.2f, 64.0f, "Backpropagation works out which way every weight should move."},
        {64.2f, 70.4f, "Repeat thousands of times. The loss keeps falling..."},
        {70.6f, 76.2f, "...and the boundary bends around the blue dots."},
        {76.4f, 79.9f, "Weights that started random now hold the answer."},
        {80.4f, 84.2f, "It can now sort dots it has never seen before."},
    };

    void Caption(float t)
    {
        for (const Cap &c : CAPS)
        {
            float a = Win(t, c.a, c.b, 0.35f);
            if (a <= 0.001f)
                continue;
            float size = 30;
            float w = g_font->MeasureText(c.text, size).x;
            Renderer::DrawRoundedRect(0, -322, w / 2 + 28, 27, 14, A(Rgb(8, 10, 16), 0.82f * a), 1.0f, 0.95f);
            Text(c.text, 0, -322, size, A(WHITE, a), TextAlign::Center, 1.0f);
        }
    }

    void Render(float t)
    {
        Renderer::SetClearColor(BG.r / 255.0f, BG.g / 255.0f, BG.b / 255.0f, 1);
        Renderer::Clear();
        Renderer::BeginScene(glm::mat4(1.0f), glm::ortho(-640.0f, 640.0f, -360.0f, 360.0f, -100.0f, 100.0f));

        const float mv = EaseIO(Seg(t, 10.5f, 12.0f));
        Panel P{Lerp(0, 330, mv), Lerp(-10, 10, mv), Lerp(440, 360, mv)};
        const float idx = IdxAt(t);
        const Net cur = NetAt(idx);
        const float sx = 0.2f, sy = 0.1f;

        float titleA = Win(t, 0.0f, 3.9f, 0.6f);
        Text("How does a neural network work?", 0, 20, 58, A(WHITE, titleA));
        Text("explained in 90 seconds", 0, -45, 28, A(MUTED, titleA * Seg(t, 0.8f, 1.6f)));

        const char *label = nullptr;
        float labelA = 0;
        struct SceneLabel { float a, b; const char *name; };
        const SceneLabel labels[] = {{3.8f, 12.0f, "1   The problem"}, {12.0f, 27.0f, "2   One neuron"}, {27.0f, 42.0f, "3   A network"}, {42.0f, 54.0f, "4   A first guess"}, {54.0f, 80.0f, "5   Learning"}, {80.0f, 85.0f, "6   It works"}};
        for (const SceneLabel &s : labels)
        {
            float a = Win(t, s.a, s.b, 0.5f);
            if (a > labelA)
            {
                labelA = a;
                label = s.name;
            }
        }
        if (label)
            Text(label, -600, 318, 30, A(AMBER, labelA), TextAlign::Left);

        const Net *map = nullptr;
        float mapAlpha = 0;
        if (t >= 42.5f)
        {
            map = &cur;
            mapAlpha = Seg(t, 42.5f, 44.0f);
        }
        DrawPanel(P, Seg(t, 3.6f, 4.4f), map, mapAlpha, Seg(t, 4.2f, 7.4f));
        if (t > 3.6f)
        {
            Text("x", P.cx + P.size / 2 - 6, P.cy - P.size / 2 - 22, 20, A(MUTED, Seg(t, 5, 6)));
            Text("y", P.cx - P.size / 2 - 20, P.cy + P.size / 2 - 8, 20, A(MUTED, Seg(t, 5, 6)));
        }

        if (t >= 12.0f && t < 27.0f)
        {
            float a = Win(t, 12.2f, 27.0f, 0.5f);
            glm::vec2 hp = ToPanel(P, 0.8f, -0.3f);
            float pulse = 0.5f + 0.5f * std::sin(t * 5.0f);
            float ha = Seg(t, 13.0f, 14.0f) * a;
            Renderer::DrawRing(hp.x, hp.y, 12 + 3 * pulse, 2.5f, A(AMBER, ha), 1.0f, 0.2f);
            Renderer::DrawCircle(hp.x, hp.y, 7, A(AMBER, ha), 1.0f, 0.19f);
            Text("(0.8, -0.3)", hp.x, hp.y + 28, 18, A(AMBER, ha));

            glm::vec2 in1(-520, 60), in2(-520, -80), nr(-300, -10);
            float ia = Seg(t, 12.8f, 13.8f) * a;
            float ea = Seg(t, 14.2f, 15.4f);
            float na = Seg(t, 14.0f, 15.0f) * a;
            for (int k = 0; k < 2; ++k)
            {
                glm::vec2 s = k == 0 ? in1 : in2;
                glm::vec2 e = s + (nr - s) * EaseIO(ea);
                Line(s.x, s.y, e.x, e.y, k == 0 ? 5.0f : 3.5f, A(k == 0 ? TEAL : PINK, 0.85f * a));
                Renderer::DrawCircle(s.x, s.y, 36, A(Mix(EDGE, WHITE, 0.4f), ia), 1.0f, 0.40f);
                Renderer::DrawCircle(s.x, s.y, 33, A(Rgb(58, 66, 98), ia), 1.0f, 0.41f);
                Text(k == 0 ? "x" : "y", s.x, s.y, 30, A(WHITE, ia));
                Text(k == 0 ? "0.8" : "-0.3", s.x - 52, s.y, 28, A(AMBER, ia), TextAlign::Right);
            }
            float wa = Seg(t, 15.4f, 16.4f) * a;
            Text("w1 = 1.2", Lerp(in1.x, nr.x, 0.45f), Lerp(in1.y, nr.y, 0.45f) + 26, 22, A(TEAL, wa));
            Text("w2 = -0.7", Lerp(in2.x, nr.x, 0.45f), Lerp(in2.y, nr.y, 0.45f) - 28, 22, A(PINK, wa));
            Renderer::DrawCircle(nr.x, nr.y, 61, A(Mix(EDGE, WHITE, 0.4f), na), 1.0f, 0.40f);
            Renderer::DrawCircle(nr.x, nr.y, 58, A(Rgb(60, 70, 120), na), 1.0f, 0.41f);
            Text("\xCE\xA3", nr.x, nr.y, 46, A(WHITE, na));
            float ba = Seg(t, 16.4f, 17.4f) * a;
            Line(nr.x, nr.y + 96, nr.x, nr.y + 62, 3, A(AMBER, ba));
            Text("bias b = 0.1", nr.x, nr.y + 112, 22, A(AMBER, ba));

            float pa = Seg(t, 20.0f, 21.0f) * a;
            Line(-232, -10, -140, -10, 4, A(MUTED, pa));
            Line(-140, -10, -154, -2, 3, A(MUTED, pa));
            Line(-140, -10, -154, -18, 3, A(MUTED, pa));
            Renderer::DrawRoundedRect(-20, -10, 100, 80, 10, A(PANEL, pa), 1.0f, 0.02f);
            Renderer::DrawRoundedRect(-20, -10, 102, 82, 11, A(EDGE, pa), 1.0f, 0.01f);
            auto sp = [&](float z) { return glm::vec2(-20 + z / 6.0f * 86, -10 - 55 + Sigmoid(z) * 110); };
            Line(-106, -10, 66, -10, 1.5f, A(EDGE, pa), 0.2f);
            Line(-20, -62, -20, 42, 1.5f, A(EDGE, pa), 0.2f);
            for (int i = 0; i < 48; ++i)
            {
                glm::vec2 u = sp(-6 + 12.0f * i / 48), v = sp(-6 + 12.0f * (i + 1) / 48);
                Line(u.x, u.y, v.x, v.y, 3.2f, A(TEAL, pa), 0.3f);
            }
            Text("squash", -20, 62, 18, A(MUTED, pa));
            float da = Seg(t, 21.0f, 22.4f) * a;
            glm::vec2 dp = sp(1.27f);
            Line(dp.x, -10, dp.x, dp.y, 2, A(AMBER, 0.8f * da), 0.25f);
            Line(-106, dp.y, dp.x, dp.y, 2, A(AMBER, 0.8f * da), 0.25f);
            Renderer::DrawCircle(dp.x, dp.y, 7, A(AMBER, da), 1.0f, 0.4f);

            Text("z = w1 \xC2\xB7 x + w2 \xC2\xB7 y + b", -600, -150, 27, A(WHITE, Seg(t, 16.8f, 17.8f) * a), TextAlign::Left);
            Text("z = 1.2\xC2\xB7" "0.8 + (-0.7)\xC2\xB7" "(-0.3) + 0.1 = 1.27", -600, -192, 27, A(WHITE, Seg(t, 18.3f, 19.5f) * a), TextAlign::Left);
            Text("a = \xCF\x83(z) = \xCF\x83(1.27) = 0.78", -600, -234, 27, A(WHITE, Seg(t, 21.2f, 22.2f) * a), TextAlign::Left);
            Text("0.78 is closer to 1 than to 0, so it leans blue", -600, -274, 25, A(BLUE, Seg(t, 24.2f, 25.2f) * a), TextAlign::Left);
        }

        if (t >= 27.0f && t < 42.0f)
        {
            float la = Win(t, 27.3f, 31.6f, 0.4f);
            if (la > 0)
            {
                glm::vec2 a1 = ToPanel(P, -0.95f, 0.95f), a2 = ToPanel(P, 0.95f, -0.95f);
                glm::vec2 e = a1 + (a2 - a1) * EaseIO(Seg(t, 27.6f, 29.0f));
                Line(a1.x, a1.y, e.x, e.y, 5, A(WHITE, la), 0.2f);
                Renderer::DrawCircle(-290, 10, 62, A(Mix(EDGE, WHITE, 0.4f), la), 1.0f, 0.40f);
                Renderer::DrawCircle(-290, 10, 59, A(Rgb(60, 70, 120), la), 1.0f, 0.41f);
                Text("\xCE\xA3", -290, 10, 46, A(WHITE, la));
                Text("one neuron", -290, -80, 24, A(MUTED, la));
                Text("one straight cut", P.cx, P.cy - P.size / 2 - 26, 22, A(WHITE, la * Seg(t, 28.5f, 29.5f)));
            }
        }

        if (t >= 31.5f && t < 85.0f)
        {
            float tr = EaseIO(Seg(t, 54.0f, 55.5f));
            NetLayout L{Lerp(-290, -475, tr), Lerp(10, 10, tr), Lerp(460, 250, tr), Lerp(330, 300, tr)};
            float a = Seg(t, 31.5f, 32.0f);
            float lit = t < 37.5f ? 0.0f : 3.0f * Seg(t, 37.5f, 41.2f);
            if (t >= 54.0f)
                lit = 3.0f;
            DrawNet(L, t < 58.0f ? g_snaps[0] : cur, a, Seg(t, 31.8f, 34.0f), Seg(t, 33.0f, 36.0f), lit, sx, sy);
        }

        if (t >= 42.0f && t < 54.0f)
        {
            const Dot &d = g_dots[g_sample];
            glm::vec2 c = ToPanel(P, d.x, d.y);
            float ra = Win(t, 44.0f, 54.0f, 0.5f);
            Renderer::DrawRing(c.x, c.y, 15, 2.5f, A(WHITE, ra), 1.0f, 0.2f);
            float q = Eval(g_snaps[0], d.x, d.y);
            float la = Seg(t, 47.4f, 48.4f) * Win(t, 47.0f, 54.0f, 0.4f);
            Text(Fmt("circled dot: truth = blue (1), guess = %.2f", q), -540, -195, 25, A(WHITE, la), TextAlign::Left);
            Text("loss =", -540, -250, 38, A(MUTED, la), TextAlign::Left);
            Text(Fmt("%.2f", g_loss[0]), -420, -250, 52, A(AMBER, la), TextAlign::Left);
            Text("(high: bad guesses)", -300, -250, 24, A(MUTED, la * Seg(t, 49.5f, 50.5f)), TextAlign::Left);
        }

        if (t >= 54.0f && t < 85.0f)
        {
            float a = Win(t, 54.0f, 80.5f, 0.5f);
            float t0 = t < 58.0f ? 1.2f : 0.9f;
            int active = static_cast<int>(std::floor((t - 54.0f) / t0)) % 3;
            const char *names[3] = {"1  Guess", "2  Measure error", "3  Nudge weights"};
            float xs[3] = {-220, -60, 100};
            float ya = t < 58.0f ? Seg(t, 54.6f, 55.4f) : 1.0f;
            for (int i = 0; i < 3; ++i)
            {
                bool on = i == active;
                Renderer::DrawRoundedRect(xs[i], -215, 72, 21, 11, A(on ? AMBER : PANEL, a * ya), 1.0f, 0.5f);
                Text(names[i], xs[i], -215, 19, A(on ? Rgb(20, 22, 30) : MUTED, a * ya), TextAlign::Center, 0.6f);
            }
            float la = Seg(t, 56.0f, 57.0f) * Win(t, 54.0f, 80.0f, 0.5f);
            Renderer::DrawRoundedRect(-90, 20, 192, 135, 12, A(EDGE, la), 1.0f, 0.00f);
            Renderer::DrawRoundedRect(-90, 20, 190, 133, 11, A(PANEL, la), 1.0f, 0.01f);
            Text("loss (error)", -270, 138, 21, A(MUTED, la), TextAlign::Left);
            Text("training steps (compressed)", -90, -100, 17, A(MUTED, la));
            const float x0 = -265, x1 = -105 + 190 - 20, y0 = -70, y1 = 105;
            Line(x0, y0, x1 + 5, y0, 1.5f, A(EDGE, la), 0.2f);
            Line(x0, y0, x0, y1 + 10, 1.5f, A(EDGE, la), 0.2f);
            int cnt = static_cast<int>(std::min(idx, static_cast<float>(g_snaps.size() - 1)));
            const float lmax = g_loss[0] * 1.05f;
            for (int k = 0; k < cnt; ++k)
            {
                const float last = static_cast<float>(g_snaps.size() - 1);
                float ax = Lerp(x0, x1, std::sqrt(k / last)), bx = Lerp(x0, x1, std::sqrt((k + 1) / last));
                float ay = Lerp(y0, y1, g_loss[k] / lmax), by = Lerp(y0, y1, g_loss[k + 1] / lmax);
                Line(ax, ay, bx, by, 3.2f, A(AMBER, la), 0.3f);
            }
            float fi = std::min(idx, static_cast<float>(g_snaps.size() - 1));
            float lv = Lerp(g_loss[std::min<int>(cnt, g_loss.size() - 1)], g_loss[std::min<int>(cnt + 1, g_loss.size() - 1)], fi - cnt);
            float px = Lerp(x0, x1, std::sqrt(fi / (g_snaps.size() - 1))), py = Lerp(y0, y1, lv / lmax);
            Renderer::DrawCircle(px, py, 8, A(WHITE, la), 1.0f, 0.4f);
            Text(Fmt("step %d     loss %.3f", static_cast<int>(fi * SNAP_EVERY), lv), -90, -165, 26, A(WHITE, la), TextAlign::Center);
        }

        if (t >= 80.0f && t < 85.0f)
        {
            float a = Win(t, 80.4f, 85.0f, 0.5f);
            Text("New dots it has never seen:", -330, 150, 26, A(MUTED, a), TextAlign::Left);
            for (size_t i = 0; i < g_test.size(); ++i)
            {
                float start = 81.0f + static_cast<float>(i) * 0.9f;
                float pa = Seg(t, start, start + 0.5f) * a;
                const Dot &d = g_test[i];
                float q = Eval(cur, d.x, d.y);
                bool blue = q > 0.5f;
                glm::vec2 c = ToPanel(P, d.x, d.y);
                float k = P.size / 360.0f;
                Renderer::DrawRing(c.x, c.y, 14 * k * Back(Seg(t, start, start + 0.5f)), 3, A(WHITE, pa), 1.0f, 0.2f);
                Renderer::DrawCircle(c.x, c.y, 7 * k, A(blue ? BLUE : ORANGE, pa), 1.0f, 0.19f);
                float y = 85 - static_cast<float>(i) * 52;
                Text(Fmt("(%.2f, %.2f)  >  %.2f  >", d.x, d.y, q), -330, y, 24, A(WHITE, pa), TextAlign::Left);
                Text(blue ? "blue" : "orange", -80, y, 24, A(blue ? BLUE : ORANGE, pa), TextAlign::Left);
                Text((blue == (d.label > 0.5f)) ? "correct" : "wrong", 25, y, 20, A(TEAL, pa), TextAlign::Left);
            }
        }

        float endA = Seg(t, 84.3f, 85.4f);
        if (endA > 0)
        {
            Renderer::DrawQuad(0, 0, W, H, 0, A(BG, endA), true, 0.97f);
            float ta = Seg(t, 85.4f, 86.6f);
            Text("A neural network is", 0, 70, 46, A(MUTED, ta), TextAlign::Center, 1.0f);
            Text("many simple neurons", 0, 0, 62, A(WHITE, ta), TextAlign::Center, 1.0f);
            Text("+ a loop of small corrections", 0, -70, 54, A(AMBER, Seg(t, 87.0f, 88.2f)), TextAlign::Center, 1.0f);
            Text("rendered with SIMPEngine", 0, -150, 20, A(MUTED, Seg(t, 88.0f, 89.0f)), TextAlign::Center, 1.0f);
        }

        Renderer::DrawQuad(-640 + 640.0f * (t / DUR), -358, 1280.0f * (t / DUR), 4, 0, A(BLUE, 0.8f), true, 1.1f);
        Caption(t);
        Renderer::EndScene();
    }
}

int main(int argc, char **argv)
{
    SIMPEngine::Log::Init();
    SIMPEngine::Window window;
    if (!window.Init("Video", 320, 180, SDL_WINDOW_HIDDEN))
        return 1;
    Renderer::Init(std::make_unique<SIMPEngine::GLRenderingAPI>(), 320, 180);
    g_font = SIMPEngine::Font::Load(SIMP_ASSETS_DIR "/fonts/Inter-SemiBold.ttf");
    if (!g_font)
        return 2;

    g_dots = MakeDots(7, 150);
    g_test = MakeDots(21, 4);
    for (size_t i = 0; i < g_dots.size(); ++i)
        if (g_dots[i].label > 0.5f && std::hypot(g_dots[i].x, g_dots[i].y) > 0.18f && std::hypot(g_dots[i].x, g_dots[i].y) < 0.4f)
        {
            g_sample = static_cast<int>(i);
            break;
        }
    Net n = Init(5, 0.9f);
    for (int e = 0; e <= EPOCHS; ++e)
    {
        if (e % SNAP_EVERY == 0)
        {
            g_snaps.push_back(n);
            g_loss.push_back(Loss(n, g_dots));
        }
        Step(n, g_dots, 1.2f);
    }
    std::fprintf(stderr, "loss %.3f -> %.4f, accuracy %.2f, test:", g_loss.front(), g_loss.back(), Accuracy(n, g_dots));
    for (const Dot &d : g_test)
        std::fprintf(stderr, " %.2f(%d)", Eval(n, d.x, d.y), d.label > 0.5f);
    std::fprintf(stderr, "\n");

    Framebuffer fb(W, H, false, 4);
    std::vector<uint8_t> pixels;
    if (argc > 2 && std::string(argv[1]) == "stills")
    {
        for (int i = 2; i < argc; ++i)
        {
            float t = static_cast<float>(std::atof(argv[i]));
            Renderer::BeginRenderTarget(fb);
            Render(t);
            fb.ReadPixels(pixels);
            Renderer::EndRenderTarget();
            SIMPEngine::ImageWriter::WritePng(Fmt("still_%02d.png", i - 2), W, H, pixels);
        }
        return 0;
    }

    std::string out = argc > 1 ? argv[1] : "neural_network.mp4";
    std::string cmd = Fmt("ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgba -s %dx%d -r %d -i - -c:v libx264 -preset medium -crf 17 -pix_fmt yuv420p -movflags +faststart \"%s\"", W, H, FPS, out.c_str());
    FILE *pipe = popen(cmd.c_str(), "w");
    if (!pipe)
        return 3;
    const int frames = static_cast<int>(DUR * FPS);
    for (int f = 0; f < frames; ++f)
    {
        Renderer::BeginRenderTarget(fb);
        Render(static_cast<float>(f) / FPS);
        fb.ReadPixels(pixels);
        Renderer::EndRenderTarget();
        std::fwrite(pixels.data(), 1, pixels.size(), pipe);
        if (f % 300 == 0)
            std::fprintf(stderr, "frame %d/%d\n", f, frames);
    }
    pclose(pipe);
    return 0;
}
