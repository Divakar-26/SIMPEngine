#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace SIMPEngine
{
    enum class CurveKind : uint8_t
    {
        Linear,
        Cubic,
        Decelerate,
        BounceIn,
        BounceOut,
        BounceInOut,
        ElasticIn,
        ElasticOut,
        ElasticInOut,
        Spring,
        Steps,
        SawTooth,
        Threshold,
        Interval,
        Flipped,
        Reversed,
        Sine
    };

    class Curve
    {
    public:
        Curve() = default;

        static Curve Linear();
        static Curve Cubic(float x1, float y1, float x2, float y2);
        static Curve Elastic(CurveKind kind, float period = 0.4f);
        static Curve Spring(float dampingRatio = 0.35f, float frequency = 3.0f);
        static Curve Steps(int count);
        static Curve SawTooth(int count);
        static Curve Threshold(float at);
        static Curve Interval(float begin, float end);
        static Curve Interval(float begin, float end, const Curve &inner);
        static Curve Flipped(const Curve &inner);
        static Curve Reversed(const Curve &inner);
        static Curve Sine(float cycles = 1.0f);

        static bool FromName(const std::string &name, Curve &out);
        static const std::vector<std::string> &Names();

        float Evaluate(float t) const;
        CurveKind GetKind() const { return m_Kind; }
        const std::string &GetName() const { return m_Name; }

    private:
        CurveKind m_Kind = CurveKind::Linear;
        float m_A = 0.0f;
        float m_B = 0.0f;
        float m_C = 0.0f;
        float m_D = 0.0f;
        std::shared_ptr<Curve> m_Inner;
        std::string m_Name = "linear";
    };
}
