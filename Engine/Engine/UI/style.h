#pragma once

#include <imgui.h>

// =====================================================================
// SIMPEngine Editor Theme — "Seamless IDE" (CLion / JetBrains New UI)
// Near-flat: dockspace and panels share almost the same background,
// hairline separators instead of colored borders, square-ish corners,
// accent used ONLY for selection/active-tab, never decoratively.
// =====================================================================

// ---- Backgrounds ------------------------------------------------------
// Editor/dockspace bg and tool-panel bg are intentionally close in value —
// that's what makes docked panels read as one continuous surface instead
// of a grid of separate boxes.
#define BG0            ImVec4(0.118f, 0.122f, 0.133f, 1.00f) // #1E1F22 dockspace / editor
#define BG1            ImVec4(0.169f, 0.176f, 0.188f, 1.00f) // #2B2D30 tool panels
#define BG2            ImVec4(0.145f, 0.149f, 0.161f, 1.00f) // #252629 inputs / controls
#define BG3            ImVec4(0.196f, 0.204f, 0.220f, 1.00f) // hovered controls
#define BG_TAB_INACTIVE ImVec4(0.153f, 0.157f, 0.169f, 1.00f) // #27282B unfocused tab

// ---- Separators — the ONLY thing marking panel boundaries -------------
#define BORDER         ImVec4(0.224f, 0.231f, 0.251f, 1.00f) // #393B40 hairline
#define BORDER_SOFT    ImVec4(0.224f, 0.231f, 0.251f, 0.60f)

// ---- Accent (JetBrains blue) — reserved for selection & active state --
#define ACCENT         ImVec4(0.208f, 0.455f, 0.941f, 1.00f) // #3574F0
#define ACCENT_HOVER   ImVec4(0.298f, 0.553f, 1.000f, 1.00f)
#define ACCENT_ACTIVE  ImVec4(0.180f, 0.373f, 0.800f, 1.00f)
#define ACCENT_MUTED   ImVec4(0.208f, 0.455f, 0.941f, 0.16f)

// ---- Text ---------------------------------------------------------------
#define TEXT           ImVec4(0.875f, 0.882f, 0.898f, 1.00f) // #DFE1E5
#define TEXT_DIM       ImVec4(0.616f, 0.627f, 0.659f, 1.00f) // #9DA0A8
#define TEXT_DISABLED  ImVec4(0.400f, 0.408f, 0.427f, 1.00f)

// ---- Status accents — sparing use only (log lines, warnings) -----------
#define SUCCESS        ImVec4(0.298f, 0.780f, 0.470f, 1.00f)
#define WARNING        ImVec4(0.965f, 0.706f, 0.243f, 1.00f)
#define DANGER         ImVec4(0.937f, 0.325f, 0.325f, 1.00f)

namespace EditorStyleMetrics
{
    // Square-ish, not rounded — rounding is what breaks the "seamless
    // panels" illusion since docked windows can't round at their shared
    // edges anyway, so mismatched rounding elsewhere just looks inconsistent.
    constexpr float WindowRounding    = 0.0f;
    constexpr float ChildRounding     = 0.0f;
    constexpr float PopupRounding     = 4.0f; // popups float, fine to round
    constexpr float FrameRounding     = 3.0f; // buttons/inputs, subtle only
    constexpr float GrabRounding      = 3.0f;
    constexpr float TabRounding       = 0.0f; // flush with docked panel edge
    constexpr float ScrollbarRounding = 4.0f;

    constexpr float FrameBorder = 0.0f; // no border box around every input —
                                         // just background-color contrast (BG2 vs BG1)
    constexpr float ChildBorder = 0.0f; // hairline only comes from DockingSeparator
    constexpr float PopupBorder = 1.0f;

    constexpr float FontSize = 14.0f; // CLion runs small/dense, not 16px
}