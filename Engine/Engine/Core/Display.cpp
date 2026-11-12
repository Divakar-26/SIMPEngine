#include <Engine/Core/Display.h>

#include <algorithm>
#include <cmath>

namespace SIMPEngine
{
    namespace
    {
        DisplaySettings s_Settings;
    }

    DisplaySettings &Display::Get()
    {
        return s_Settings;
    }

    void Display::Set(const DisplaySettings &settings)
    {
        s_Settings = settings;
        s_Settings.Width = std::max(64, settings.Width);
        s_Settings.Height = std::max(64, settings.Height);
        s_Settings.RenderScale = std::clamp(settings.RenderScale, 0.2f, 1.0f);
    }

    void Display::Reset()
    {
        s_Settings = DisplaySettings{};
    }

    const char *Display::AspectName(AspectMode mode)
    {
        switch (mode)
        {
        case AspectMode::KeepWidth:
            return "Keep Width";
        case AspectMode::Expand:
            return "Expand";
        case AspectMode::Keep:
            return "Keep (black bars)";
        default:
            return "Keep Height";
        }
    }

    const char *Display::StretchName(StretchMode mode)
    {
        return mode == StretchMode::Fixed ? "Game resolution" : "Window resolution";
    }

    glm::vec2 Display::HalfExtents(const DisplaySettings &settings, float zoom, float aspect)
    {
        const float designAspect = static_cast<float>(settings.Width) / static_cast<float>(settings.Height);
        bool byHeight = true;
        switch (settings.Aspect)
        {
        case AspectMode::KeepWidth:
            byHeight = false;
            break;
        case AspectMode::Expand:
            byHeight = aspect >= designAspect;
            break;
        default:
            break;
        }
        if (byHeight)
        {
            const float halfHeight = static_cast<float>(settings.Height) * 0.5f * zoom;
            return {halfHeight * aspect, halfHeight};
        }
        const float halfWidth = static_cast<float>(settings.Width) * 0.5f * zoom;
        return {halfWidth, halfWidth / aspect};
    }

    ViewLayout Display::Compute(int windowWidth, int windowHeight)
    {
        return Compute(s_Settings, windowWidth, windowHeight);
    }

    ViewLayout Display::Compute(const DisplaySettings &settings, int windowWidth, int windowHeight)
    {
        ViewLayout layout;
        layout.WindowWidth = std::max(1, windowWidth);
        layout.WindowHeight = std::max(1, windowHeight);
        const float windowAspect = static_cast<float>(layout.WindowWidth) / static_cast<float>(layout.WindowHeight);
        const float designAspect = static_cast<float>(settings.Width) / static_cast<float>(settings.Height);

        layout.X = 0;
        layout.Y = 0;
        layout.Width = layout.WindowWidth;
        layout.Height = layout.WindowHeight;

        if (settings.Aspect == AspectMode::Keep)
        {
            if (windowAspect > designAspect)
            {
                layout.Height = layout.WindowHeight;
                layout.Width = std::max(1, static_cast<int>(std::lround(layout.WindowHeight * designAspect)));
            }
            else
            {
                layout.Width = layout.WindowWidth;
                layout.Height = std::max(1, static_cast<int>(std::lround(layout.WindowWidth / designAspect)));
            }
            layout.X = (layout.WindowWidth - layout.Width) / 2;
            layout.Y = (layout.WindowHeight - layout.Height) / 2;
            layout.Offscreen = layout.Width != layout.WindowWidth || layout.Height != layout.WindowHeight;
        }

        layout.RenderWidth = layout.Width;
        layout.RenderHeight = layout.Height;

        if (settings.Stretch == StretchMode::Fixed)
        {
            const float aspect = static_cast<float>(layout.Width) / static_cast<float>(layout.Height);
            bool byHeight = true;
            if (settings.Aspect == AspectMode::KeepWidth)
                byHeight = false;
            else if (settings.Aspect == AspectMode::Expand)
                byHeight = aspect >= designAspect;
            if (byHeight)
            {
                layout.RenderHeight = settings.Height;
                layout.RenderWidth = std::max(1, static_cast<int>(std::lround(settings.Height * aspect)));
            }
            else
            {
                layout.RenderWidth = settings.Width;
                layout.RenderHeight = std::max(1, static_cast<int>(std::lround(settings.Width / aspect)));
            }
            layout.Offscreen = true;
        }

        const float scale = std::clamp(settings.RenderScale, 0.2f, 1.0f);
        if (scale < 0.999f)
        {
            layout.RenderWidth = std::max(1, static_cast<int>(std::lround(layout.RenderWidth * scale)));
            layout.RenderHeight = std::max(1, static_cast<int>(std::lround(layout.RenderHeight * scale)));
            layout.Offscreen = true;
        }
        return layout;
    }
}
