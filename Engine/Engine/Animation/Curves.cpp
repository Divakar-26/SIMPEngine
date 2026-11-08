#include "Curves.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>

namespace SIMPEngine
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;

        struct NamedCubic
        {
            const char *Name;
            float X1, Y1, X2, Y2;
        };

        const NamedCubic Cubics[] = {
            {"ease", 0.25f, 0.1f, 0.25f, 1.0f},
            {"easeIn", 0.42f, 0.0f, 1.0f, 1.0f},
            {"easeInToLinear", 0.67f, 0.03f, 0.65f, 0.09f},
            {"easeInSine", 0.47f, 0.0f, 0.745f, 0.715f},
            {"easeInQuad", 0.55f, 0.085f, 0.68f, 0.53f},
            {"easeInCubic", 0.55f, 0.055f, 0.675f, 0.19f},
            {"easeInQuart", 0.895f, 0.03f, 0.685f, 0.22f},
            {"easeInQuint", 0.755f, 0.05f, 0.855f, 0.06f},
            {"easeInExpo", 0.95f, 0.05f, 0.795f, 0.035f},
            {"easeInCirc", 0.6f, 0.04f, 0.98f, 0.335f},
            {"easeInBack", 0.6f, -0.28f, 0.735f, 0.045f},
            {"easeOut", 0.0f, 0.0f, 0.58f, 1.0f},
            {"linearToEaseOut", 0.35f, 0.91f, 0.33f, 0.97f},
            {"easeOutSine", 0.39f, 0.575f, 0.565f, 1.0f},
            {"easeOutQuad", 0.25f, 0.46f, 0.45f, 0.94f},
            {"easeOutCubic", 0.215f, 0.61f, 0.355f, 1.0f},
            {"easeOutQuart", 0.165f, 0.84f, 0.44f, 1.0f},
            {"easeOutQuint", 0.23f, 1.0f, 0.32f, 1.0f},
            {"easeOutExpo", 0.19f, 1.0f, 0.22f, 1.0f},
            {"easeOutCirc", 0.075f, 0.82f, 0.165f, 1.0f},
            {"easeOutBack", 0.175f, 0.885f, 0.32f, 1.275f},
            {"easeInOut", 0.42f, 0.0f, 0.58f, 1.0f},
            {"easeInOutSine", 0.445f, 0.05f, 0.55f, 0.95f},
            {"easeInOutQuad", 0.455f, 0.03f, 0.515f, 0.955f},
            {"easeInOutCubic", 0.645f, 0.045f, 0.355f, 1.0f},
            {"easeInOutQuart", 0.77f, 0.0f, 0.175f, 1.0f},
            {"easeInOutQuint", 0.86f, 0.0f, 0.07f, 1.0f},
            {"easeInOutExpo", 1.0f, 0.0f, 0.0f, 1.0f},
            {"easeInOutCirc", 0.785f, 0.135f, 0.15f, 0.86f},
            {"easeInOutBack", 0.68f, -0.55f, 0.265f, 1.55f},
            {"fastOutSlowIn", 0.4f, 0.0f, 0.2f, 1.0f},
            {"slowMiddle", 0.15f, 0.85f, 0.85f, 0.15f},
            {"fastLinearToSlowEaseIn", 0.18f, 1.0f, 0.04f, 1.0f},
            {"standard", 0.2f, 0.0f, 0.0f, 1.0f},
            {"standardAccelerate", 0.3f, 0.0f, 1.0f, 1.0f},
            {"standardDecelerate", 0.0f, 0.0f, 0.0f, 1.0f},
            {"emphasizedDecelerate", 0.05f, 0.7f, 0.1f, 1.0f},
            {"emphasizedAccelerate", 0.3f, 0.0f, 0.8f, 0.15f},
        };

        std::string Lower(std::string s)
        {
            for (char &c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }

        float CubicComponent(float a, float b, float m)
        {
            return 3.0f * a * (1.0f - m) * (1.0f - m) * m + 3.0f * b * (1.0f - m) * m * m + m * m * m;
        }

        float EvaluateCubic(float x1, float y1, float x2, float y2, float t)
        {
            float start = 0.0f;
            float end = 1.0f;
            for (int i = 0; i < 40; ++i)
            {
                const float midpoint = (start + end) * 0.5f;
                const float estimate = CubicComponent(x1, x2, midpoint);
                if (std::abs(t - estimate) < 0.0001f)
                    return CubicComponent(y1, y2, midpoint);
                if (estimate < t)
                    start = midpoint;
                else
                    end = midpoint;
            }
            return CubicComponent(y1, y2, (start + end) * 0.5f);
        }

        float Bounce(float t)
        {
            if (t < 1.0f / 2.75f)
                return 7.5625f * t * t;
            if (t < 2.0f / 2.75f)
            {
                t -= 1.5f / 2.75f;
                return 7.5625f * t * t + 0.75f;
            }
            if (t < 2.5f / 2.75f)
            {
                t -= 2.25f / 2.75f;
                return 7.5625f * t * t + 0.9375f;
            }
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }

        float SpringRaw(float zeta, float frequency, float t)
        {
            const float omega = 2.0f * Pi * frequency;
            const float z = std::clamp(zeta, 0.01f, 0.99f);
            const float damped = omega * std::sqrt(1.0f - z * z);
            const float envelope = std::exp(-z * omega * t);
            return 1.0f - envelope * (std::cos(damped * t) + (z * omega / damped) * std::sin(damped * t));
        }
    }

    Curve Curve::Linear()
    {
        return Curve();
    }

    Curve Curve::Cubic(float x1, float y1, float x2, float y2)
    {
        Curve c;
        c.m_Kind = CurveKind::Cubic;
        c.m_A = x1;
        c.m_B = y1;
        c.m_C = x2;
        c.m_D = y2;
        c.m_Name = "cubic";
        return c;
    }

    Curve Curve::Elastic(CurveKind kind, float period)
    {
        Curve c;
        c.m_Kind = kind;
        c.m_A = std::max(0.01f, period);
        c.m_Name = kind == CurveKind::ElasticIn ? "elasticIn" : kind == CurveKind::ElasticOut ? "elasticOut" : "elasticInOut";
        return c;
    }

    Curve Curve::Spring(float dampingRatio, float frequency)
    {
        Curve c;
        c.m_Kind = CurveKind::Spring;
        c.m_A = dampingRatio;
        c.m_B = std::max(0.1f, frequency);
        c.m_Name = "spring";
        return c;
    }

    Curve Curve::Steps(int count)
    {
        Curve c;
        c.m_Kind = CurveKind::Steps;
        c.m_A = static_cast<float>(std::max(1, count));
        c.m_Name = "steps";
        return c;
    }

    Curve Curve::SawTooth(int count)
    {
        Curve c;
        c.m_Kind = CurveKind::SawTooth;
        c.m_A = static_cast<float>(std::max(1, count));
        c.m_Name = "sawTooth";
        return c;
    }

    Curve Curve::Threshold(float at)
    {
        Curve c;
        c.m_Kind = CurveKind::Threshold;
        c.m_A = at;
        c.m_Name = "threshold";
        return c;
    }

    Curve Curve::Interval(float begin, float end)
    {
        return Interval(begin, end, Curve());
    }

    Curve Curve::Interval(float begin, float end, const Curve &inner)
    {
        Curve c;
        c.m_Kind = CurveKind::Interval;
        c.m_A = std::clamp(begin, 0.0f, 1.0f);
        c.m_B = std::clamp(end, c.m_A, 1.0f);
        c.m_Inner = std::make_shared<Curve>(inner);
        c.m_Name = "interval";
        return c;
    }

    Curve Curve::Flipped(const Curve &inner)
    {
        Curve c;
        c.m_Kind = CurveKind::Flipped;
        c.m_Inner = std::make_shared<Curve>(inner);
        c.m_Name = "flipped";
        return c;
    }

    Curve Curve::Reversed(const Curve &inner)
    {
        Curve c;
        c.m_Kind = CurveKind::Reversed;
        c.m_Inner = std::make_shared<Curve>(inner);
        c.m_Name = "reversed";
        return c;
    }

    Curve Curve::Sine(float cycles)
    {
        Curve c;
        c.m_Kind = CurveKind::Sine;
        c.m_A = cycles;
        c.m_Name = "sine";
        return c;
    }

    const std::vector<std::string> &Curve::Names()
    {
        static const std::vector<std::string> names = []
        {
            std::vector<std::string> list = {"linear", "decelerate", "bounceIn", "bounceOut", "bounceInOut", "elasticIn", "elasticOut", "elasticInOut", "spring"};
            for (const NamedCubic &cubic : Cubics)
                list.push_back(cubic.Name);
            return list;
        }();
        return names;
    }

    bool Curve::FromName(const std::string &name, Curve &out)
    {
        static const std::unordered_map<std::string, Curve> table = []
        {
            std::unordered_map<std::string, Curve> map;
            auto add = [&](const std::string &key, Curve curve)
            {
                curve.m_Name = key;
                map[Lower(key)] = curve;
            };
            add("linear", Curve());
            Curve decelerate;
            decelerate.m_Kind = CurveKind::Decelerate;
            add("decelerate", decelerate);
            for (CurveKind kind : {CurveKind::BounceIn, CurveKind::BounceOut, CurveKind::BounceInOut})
            {
                Curve bounce;
                bounce.m_Kind = kind;
                add(kind == CurveKind::BounceIn ? "bounceIn" : kind == CurveKind::BounceOut ? "bounceOut" : "bounceInOut", bounce);
            }
            add("elasticIn", Elastic(CurveKind::ElasticIn));
            add("elasticOut", Elastic(CurveKind::ElasticOut));
            add("elasticInOut", Elastic(CurveKind::ElasticInOut));
            add("spring", Spring());
            for (const NamedCubic &cubic : Cubics)
                add(cubic.Name, Cubic(cubic.X1, cubic.Y1, cubic.X2, cubic.Y2));
            return map;
        }();

        auto it = table.find(Lower(name));
        if (it == table.end())
            return false;
        out = it->second;
        return true;
    }

    float Curve::Evaluate(float t) const
    {
        if (m_Kind == CurveKind::Sine)
            return std::sin(2.0f * Pi * m_A * t);

        t = std::clamp(t, 0.0f, 1.0f);
        if (m_Kind != CurveKind::Interval && m_Kind != CurveKind::Threshold && m_Kind != CurveKind::Flipped && m_Kind != CurveKind::Reversed)
        {
            if (t <= 0.0f)
                return 0.0f;
            if (t >= 1.0f)
                return 1.0f;
        }

        switch (m_Kind)
        {
        case CurveKind::Linear:
            return t;
        case CurveKind::Cubic:
            return EvaluateCubic(m_A, m_B, m_C, m_D, t);
        case CurveKind::Decelerate:
            return 1.0f - (1.0f - t) * (1.0f - t);
        case CurveKind::BounceIn:
            return 1.0f - Bounce(1.0f - t);
        case CurveKind::BounceOut:
            return Bounce(t);
        case CurveKind::BounceInOut:
            return t < 0.5f ? (1.0f - Bounce(1.0f - t * 2.0f)) * 0.5f : Bounce(t * 2.0f - 1.0f) * 0.5f + 0.5f;
        case CurveKind::ElasticIn:
        {
            const float s = m_A / 4.0f;
            const float u = t - 1.0f;
            return -std::pow(2.0f, 10.0f * u) * std::sin((u - s) * (Pi * 2.0f) / m_A);
        }
        case CurveKind::ElasticOut:
        {
            const float s = m_A / 4.0f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t - s) * (Pi * 2.0f) / m_A) + 1.0f;
        }
        case CurveKind::ElasticInOut:
        {
            const float s = m_A / 4.0f;
            const float u = 2.0f * t - 1.0f;
            if (u < 0.0f)
                return -0.5f * std::pow(2.0f, 10.0f * u) * std::sin((u - s) * (Pi * 2.0f) / m_A);
            return std::pow(2.0f, -10.0f * u) * std::sin((u - s) * (Pi * 2.0f) / m_A) * 0.5f + 1.0f;
        }
        case CurveKind::Spring:
        {
            const float end = SpringRaw(m_A, m_B, 1.0f);
            return SpringRaw(m_A, m_B, t) + (1.0f - end) * t;
        }
        case CurveKind::Steps:
            return std::floor(t * m_A) / m_A;
        case CurveKind::SawTooth:
        {
            const float v = t * m_A;
            return v - std::floor(v);
        }
        case CurveKind::Threshold:
            return t < m_A ? 0.0f : 1.0f;
        case CurveKind::Interval:
        {
            if (t <= m_A)
                return m_Inner ? m_Inner->Evaluate(0.0f) : 0.0f;
            if (t >= m_B)
                return m_Inner ? m_Inner->Evaluate(1.0f) : 1.0f;
            const float local = (t - m_A) / std::max(m_B - m_A, 1e-6f);
            return m_Inner ? m_Inner->Evaluate(local) : local;
        }
        case CurveKind::Flipped:
            return 1.0f - (m_Inner ? m_Inner->Evaluate(1.0f - t) : 1.0f - t);
        case CurveKind::Reversed:
            return m_Inner ? m_Inner->Evaluate(1.0f - t) : 1.0f - t;
        case CurveKind::Sine:
            break;
        }
        return t;
    }
}
