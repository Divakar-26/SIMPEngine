#pragma once

#include <glm/glm.hpp>

namespace SIMPEngine
{
    enum class AspectMode
    {
        KeepHeight,
        KeepWidth,
        Expand,
        Keep
    };

    enum class StretchMode
    {
        Window,
        Fixed
    };

    struct DisplaySettings
    {
        int Width = 1920;
        int Height = 1080;
        AspectMode Aspect = AspectMode::KeepHeight;
        StretchMode Stretch = StretchMode::Window;
        float RenderScale = 1.0f;
    };

    struct ViewLayout
    {
        int WindowWidth = 0;
        int WindowHeight = 0;
        int X = 0;
        int Y = 0;
        int Width = 0;
        int Height = 0;
        int RenderWidth = 0;
        int RenderHeight = 0;
        bool Offscreen = false;
    };

    class Display
    {
    public:
        static DisplaySettings &Get();
        static void Set(const DisplaySettings &settings);
        static void Reset();

        static ViewLayout Compute(int windowWidth, int windowHeight);
        static ViewLayout Compute(const DisplaySettings &settings, int windowWidth, int windowHeight);
        static glm::vec2 HalfExtents(const DisplaySettings &settings, float zoom, float aspect);
        static const char *AspectName(AspectMode mode);
        static const char *StretchName(StretchMode mode);
    };
}
