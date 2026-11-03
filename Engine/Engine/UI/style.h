#pragma once

#include <imgui.h>

#define BG0            ImVec4(0.118f, 0.122f, 0.133f, 1.00f)
#define BG1            ImVec4(0.169f, 0.176f, 0.188f, 1.00f)
#define BG2            ImVec4(0.145f, 0.149f, 0.161f, 1.00f)
#define BG3            ImVec4(0.196f, 0.204f, 0.220f, 1.00f)
#define BG_TAB_INACTIVE ImVec4(0.153f, 0.157f, 0.169f, 1.00f)

#define BORDER         ImVec4(0.224f, 0.231f, 0.251f, 1.00f)
#define BORDER_SOFT    ImVec4(0.224f, 0.231f, 0.251f, 0.60f)

#define ACCENT         ImVec4(0.208f, 0.455f, 0.941f, 1.00f)
#define ACCENT_HOVER   ImVec4(0.298f, 0.553f, 1.000f, 1.00f)
#define ACCENT_ACTIVE  ImVec4(0.180f, 0.373f, 0.800f, 1.00f)
#define ACCENT_MUTED   ImVec4(0.208f, 0.455f, 0.941f, 0.16f)

#define TEXT           ImVec4(0.875f, 0.882f, 0.898f, 1.00f)
#define TEXT_DIM       ImVec4(0.616f, 0.627f, 0.659f, 1.00f)
#define TEXT_DISABLED  ImVec4(0.400f, 0.408f, 0.427f, 1.00f)

#define SUCCESS        ImVec4(0.298f, 0.780f, 0.470f, 1.00f)
#define WARNING        ImVec4(0.965f, 0.706f, 0.243f, 1.00f)
#define DANGER         ImVec4(0.937f, 0.325f, 0.325f, 1.00f)

namespace EditorStyleMetrics
{
    constexpr float WindowRounding    = 0.0f;
    constexpr float ChildRounding     = 0.0f;
    constexpr float PopupRounding     = 4.0f;
    constexpr float FrameRounding     = 3.0f;
    constexpr float GrabRounding      = 3.0f;
    constexpr float TabRounding       = 0.0f;
    constexpr float ScrollbarRounding = 4.0f;

    constexpr float FrameBorder = 0.0f;
    constexpr float ChildBorder = 0.0f;
    constexpr float PopupBorder = 1.0f;

    constexpr float FontSize = 14.0f;
}
