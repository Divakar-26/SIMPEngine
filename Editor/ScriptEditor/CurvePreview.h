#pragma once

#include "CodeTheme.h"

#include <imgui.h>

#include <string>

namespace ScriptEditor
{
    ImVec2 CurvePreviewSize(ImFont *font, float fontSize);

    bool DrawCurvePreview(ImDrawList *dl, ImFont *font, float fontSize, const CodeTheme &theme, ImVec2 pos, const std::string &name);
}
