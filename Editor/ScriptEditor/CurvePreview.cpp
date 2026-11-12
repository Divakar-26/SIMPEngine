#define IMGUI_DEFINE_MATH_OPERATORS
#include "CurvePreview.h"

#include <Engine/Animation/Curves.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace ScriptEditor
{
    ImVec2 CurvePreviewSize(ImFont *, float fontSize)
    {
        return ImVec2(std::floor(fontSize * 11.5f), std::floor(fontSize * 9.5f));
    }

    bool DrawCurvePreview(ImDrawList *dl, ImFont *font, float fontSize, const CodeTheme &theme, ImVec2 pos, const std::string &name)
    {
        SIMPEngine::Curve curve;
        if (!SIMPEngine::Curve::FromName(name, curve))
            return false;

        const ImVec2 size = CurvePreviewSize(font, fontSize);
        const ImVec2 max(pos.x + size.x, pos.y + size.y);
        dl->AddRectFilled(pos + ImVec2(2, 3), max + ImVec2(2, 3), IM_COL32(0, 0, 0, 60), 6.0f);
        dl->AddRectFilled(pos, max, theme.PopupBackground, 6.0f);
        dl->AddRect(pos, max, theme.PopupBorder, 6.0f);

        const ImU32 faint = (theme.PopupForeground & 0x00FFFFFF) | (70u << IM_COL32_A_SHIFT);
        const ImU32 accent = theme.PopupHighlight;

        const float titleHeight = std::floor(fontSize * 1.7f);
        const std::string &shown = curve.GetName().empty() ? name : curve.GetName();
        dl->AddText(font, fontSize * 0.95f, ImVec2(pos.x + 10.0f, pos.y + (titleHeight - fontSize) * 0.5f + 2.0f), theme.PopupForeground, shown.c_str());

        const float pad = 12.0f;
        const ImVec2 plotMin(pos.x + pad + 4.0f, pos.y + titleHeight + 6.0f);
        const ImVec2 plotMax(max.x - pad - 4.0f, max.y - pad);
        const float plotW = plotMax.x - plotMin.x;
        const float plotH = plotMax.y - plotMin.y;

        constexpr int Samples = 72;
        float values[Samples + 1];
        float low = 0.0f;
        float high = 1.0f;
        for (int i = 0; i <= Samples; ++i)
        {
            values[i] = curve.Evaluate(static_cast<float>(i) / static_cast<float>(Samples));
            low = std::min(low, values[i]);
            high = std::max(high, values[i]);
        }
        const float span = std::max(high - low, 0.001f);
        const float margin = span * 0.06f;
        low -= margin;
        high += margin;

        auto toScreen = [&](float t, float v)
        {
            return ImVec2(plotMin.x + t * plotW, plotMax.y - (v - low) / (high - low) * plotH);
        };

        dl->AddLine(toScreen(0.0f, 0.0f), toScreen(1.0f, 0.0f), faint);
        dl->AddLine(toScreen(0.0f, 1.0f), toScreen(1.0f, 1.0f), faint);
        dl->AddLine(toScreen(0.0f, 0.0f), toScreen(0.0f, 1.0f), faint);
        dl->AddLine(toScreen(1.0f, 0.0f), toScreen(1.0f, 1.0f), faint);

        std::vector<ImVec2> points;
        points.reserve(Samples + 1);
        for (int i = 0; i <= Samples; ++i)
            points.push_back(toScreen(static_cast<float>(i) / static_cast<float>(Samples), values[i]));
        dl->AddPolyline(points.data(), static_cast<int>(points.size()), accent, ImDrawFlags_None, 2.0f);

        const float cycle = 1.8f;
        const float phase = static_cast<float>(std::fmod(ImGui::GetTime(), static_cast<double>(cycle))) / cycle;
        const float t = std::clamp(phase / 0.75f, 0.0f, 1.0f);
        const float value = curve.Evaluate(t);
        const ImVec2 dot = toScreen(t, value);
        dl->AddCircleFilled(dot, 4.0f, accent);
        dl->AddCircle(dot, 4.0f, theme.PopupBackground, 0, 1.5f);

        const ImVec2 railA(plotMin.x, max.y - 5.0f);
        const ImVec2 railB(plotMax.x, max.y - 5.0f);
        dl->AddLine(railA, railB, faint, 1.0f);
        dl->AddCircleFilled(ImVec2(railA.x + (railB.x - railA.x) * value, railA.y), 3.0f, accent);
        return true;
    }
}
