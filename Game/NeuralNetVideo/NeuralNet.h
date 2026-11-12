#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

struct Net
{
    float W1[4][2], b1[4], W2[4], b2;
};

struct Dot
{
    float x, y, label;
};

inline float Sigmoid(float z) { return 1.0f / (1.0f + std::exp(-z)); }

inline float Eval(const Net &n, float x, float y, float *hidden = nullptr, float *z2out = nullptr)
{
    float z2 = n.b2;
    for (int i = 0; i < 4; ++i)
    {
        float h = std::tanh(n.W1[i][0] * x + n.W1[i][1] * y + n.b1[i]);
        if (hidden)
            hidden[i] = h;
        z2 += n.W2[i] * h;
    }
    if (z2out)
        *z2out = z2;
    return Sigmoid(z2);
}

inline float Loss(const Net &n, const std::vector<Dot> &d)
{
    float s = 0;
    for (const Dot &p : d)
    {
        float q = std::fmin(std::fmax(Eval(n, p.x, p.y), 1e-6f), 1 - 1e-6f);
        s += -(p.label * std::log(q) + (1 - p.label) * std::log(1 - q));
    }
    return s / static_cast<float>(d.size());
}

inline float Accuracy(const Net &n, const std::vector<Dot> &d)
{
    int ok = 0;
    for (const Dot &p : d)
        ok += ((Eval(n, p.x, p.y) > 0.5f) == (p.label > 0.5f));
    return static_cast<float>(ok) / static_cast<float>(d.size());
}

struct Rng
{
    uint32_t s;
    float Next() { s = s * 1664525u + 1013904223u; return static_cast<float>(s >> 8) / 16777216.0f; }
    float Normal() { float a = Next() + 1e-7f, b = Next(); return std::sqrt(-2 * std::log(a)) * std::cos(6.2831853f * b); }
};

inline std::vector<Dot> MakeDots(uint32_t seed, int count)
{
    Rng r{seed};
    std::vector<Dot> d;
    while (static_cast<int>(d.size()) < count)
    {
        float x = r.Next() * 2 - 1, y = r.Next() * 2 - 1;
        float dist = std::sqrt(x * x + y * y);
        if (dist > 0.53f && dist < 0.69f)
            continue;
        d.push_back({x, y, dist < 0.61f ? 1.0f : 0.0f});
    }
    return d;
}

inline Net Init(uint32_t seed, float scale)
{
    Rng r{seed};
    Net n{};
    for (int i = 0; i < 4; ++i)
    {
        n.W1[i][0] = r.Normal() * scale;
        n.W1[i][1] = r.Normal() * scale;
        n.b1[i] = r.Normal() * scale * 0.5f;
        n.W2[i] = r.Normal() * scale;
    }
    n.b2 = 0;
    return n;
}

inline void Step(Net &n, const std::vector<Dot> &d, float lr)
{
    Net g{};
    for (const Dot &p : d)
    {
        float h[4];
        float q = Eval(n, p.x, p.y, h);
        float dz2 = q - p.label;
        g.b2 += dz2;
        for (int i = 0; i < 4; ++i)
        {
            g.W2[i] += dz2 * h[i];
            float dz1 = dz2 * n.W2[i] * (1 - h[i] * h[i]);
            g.W1[i][0] += dz1 * p.x;
            g.W1[i][1] += dz1 * p.y;
            g.b1[i] += dz1;
        }
    }
    float k = lr / static_cast<float>(d.size());
    n.b2 -= k * g.b2;
    for (int i = 0; i < 4; ++i)
    {
        n.W2[i] -= k * g.W2[i];
        n.W1[i][0] -= k * g.W1[i][0];
        n.W1[i][1] -= k * g.W1[i][1];
        n.b1[i] -= k * g.b1[i];
    }
}
