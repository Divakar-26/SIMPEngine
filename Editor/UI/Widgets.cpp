#define IMGUI_DEFINE_MATH_OPERATORS
#include "Widgets.h"

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace EditorUI
{
    namespace
    {
        int s_TreeDepth = 0;
        std::function<void()> s_EditHook;
        int s_EditSuppression = 0;

        constexpr float LabelRatio = 0.40f;
        constexpr float LabelMin = 70.0f;
        constexpr float LabelMax = 140.0f;

        void DrawIcon(ImDrawList *dl, const char *icon, const ImVec2 &center, ImU32 color, float size)
        {
            ImFont *font = ImGui::GetFont();
            ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, icon);
            dl->AddText(font, size, ImVec2(std::floor(center.x - ts.x * 0.5f), std::floor(center.y - ts.y * 0.5f)), color, icon);
        }

        void DrawTextClipped(ImDrawList *dl, ImFont *font, float size, const ImVec2 &pos, float maxX, ImU32 color, const char *text)
        {
            const char *end = text + std::strlen(text);
            ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text, end);
            if (pos.x + ts.x <= maxX)
            {
                dl->AddText(font, size, pos, color, text, end);
                return;
            }

            const char *ellipsis = "...";
            float ellipsisW = font->CalcTextSizeA(size, FLT_MAX, 0.0f, ellipsis).x;
            float avail = maxX - pos.x - ellipsisW;
            if (avail <= 0.0f)
                return;

            const char *cut = text;
            float w = 0.0f;
            while (cut < end)
            {
                unsigned int c = 0;
                int len = ImTextCharFromUtf8(&c, cut, end);
                if (len <= 0)
                    break;
                float cw = font->CalcTextSizeA(size, FLT_MAX, 0.0f, cut, cut + len).x;
                if (w + cw > avail)
                    break;
                w += cw;
                cut += len;
            }

            dl->AddText(font, size, pos, color, text, cut);
            dl->AddText(font, size, ImVec2(pos.x + w, pos.y), color, ellipsis);
        }

        float LabelWidth()
        {
            float avail = ImGui::GetContentRegionAvail().x;
            return std::clamp(std::floor(avail * LabelRatio), LabelMin, LabelMax);
        }

        bool Edited(bool changed)
        {
            if (changed)
                NotifyEdit();
            return changed;
        }
    }

    void SetEditHook(std::function<void()> hook)
    {
        s_EditHook = std::move(hook);
    }

    void NotifyEdit()
    {
        if (s_EditHook && s_EditSuppression == 0)
            s_EditHook();
    }

    void PushEditSuppression()
    {
        ++s_EditSuppression;
    }

    void PopEditSuppression()
    {
        if (s_EditSuppression > 0)
            --s_EditSuppression;
    }

    void Tooltip(const char *text)
    {
        if (!text || !ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            return;
        ImGui::SetTooltip("%s", text);
    }

    bool ContainsInsensitive(const char *haystack, const char *needle)
    {
        if (!needle || !*needle)
            return true;
        if (!haystack)
            return false;

        size_t nl = std::strlen(needle);
        for (const char *h = haystack; *h; ++h)
        {
            size_t i = 0;
            while (i < nl && h[i] && std::tolower(static_cast<unsigned char>(h[i])) == std::tolower(static_cast<unsigned char>(needle[i])))
                ++i;
            if (i == nl)
                return true;
        }
        return false;
    }

    bool IconButton(const char *id, const char *icon, const char *tooltip, bool active, float size)
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        float s = size > 0.0f ? size : ImGui::GetFrameHeight();
        ImGuiID gid = window->GetID(id);
        ImVec2 pos = window->DC.CursorPos;
        ImRect bb(pos, pos + ImVec2(s, s));

        ImGui::ItemSize(bb, ImGui::GetStyle().FramePadding.y);
        if (!ImGui::ItemAdd(bb, gid))
            return false;

        bool hovered = false, held = false;
        bool pressed = ImGui::ButtonBehavior(bb, gid, &hovered, &held);

        ImDrawList *dl = window->DrawList;
        if (active)
            dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Selection), Metrics::FrameRounding);
        else if (held)
            dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::HeaderHover), Metrics::FrameRounding);
        else if (hovered)
            dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Hover), Metrics::FrameRounding);

        ImVec4 fg = active ? Colors::Text : (hovered ? Colors::Text : Colors::TextDim);
        DrawIcon(dl, icon, bb.GetCenter(), U32(fg), ImGui::GetFontSize());

        Tooltip(tooltip);
        return pressed;
    }

    bool Button(const char *label, ButtonKind kind, const ImVec2 &sizeArg)
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        const ImGuiStyle &style = ImGui::GetStyle();
        ImGuiID id = window->GetID(label);
        const char *labelEnd = ImGui::FindRenderedTextEnd(label);
        ImVec2 labelSize = ImGui::CalcTextSize(label, labelEnd, false);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size = ImGui::CalcItemSize(sizeArg, labelSize.x + style.FramePadding.x * 2.0f + 8.0f, ImGui::GetFrameHeight());
        ImRect bb(pos, pos + size);

        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool hovered = false, held = false;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        ImVec4 bg, fg = Colors::Text;
        switch (kind)
        {
        case ButtonKind::Primary:
            bg = held || hovered ? Colors::AccentHover : Colors::Accent;
            fg = Colors::TextOnAccent;
            break;
        case ButtonKind::Ghost:
            bg = held ? Colors::HeaderHover : hovered ? Colors::Hover : ImVec4(0, 0, 0, 0);
            fg = hovered ? Colors::Text : Colors::TextDim;
            break;
        case ButtonKind::Danger:
            bg = held || hovered ? Colors::HeaderHover : Colors::Header;
            fg = Colors::Error;
            break;
        default:
            bg = held ? Colors::BorderStrong : hovered ? Colors::HeaderHover : Colors::Header;
            break;
        }

        ImDrawList *dl = window->DrawList;
        if (bg.w > 0.0f)
            dl->AddRectFilled(bb.Min, bb.Max, U32(bg), Metrics::FrameRounding);
        if (kind == ButtonKind::Secondary || kind == ButtonKind::Danger)
            dl->AddRect(bb.Min, bb.Max, U32(Colors::Border), Metrics::FrameRounding);

        ImVec2 tp(std::floor(bb.GetCenter().x - labelSize.x * 0.5f), std::floor(bb.GetCenter().y - labelSize.y * 0.5f));
        dl->AddText(tp, U32(fg), label, labelEnd);

        return pressed;
    }

    bool SearchBox(const char *id, char *buffer, size_t bufferSize, const char *hint, float width)
    {
        const ImGuiStyle &style = ImGui::GetStyle();
        float iconW = ImGui::CalcTextSize(ICON_LC_SEARCH).x;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x + iconW + 6.0f, style.FramePadding.y));
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::SetNextItemWidth(width);
        bool changed = ImGui::InputTextWithHint(id, hint, buffer, bufferSize);
        ImGui::PopStyleVar();

        ImGui::GetWindowDrawList()->AddText(ImVec2(pos.x + style.FramePadding.x, pos.y + style.FramePadding.y), U32(Colors::TextFaint), ICON_LC_SEARCH);
        return changed;
    }

    void SectionLabel(const char *text)
    {
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
    }

    void EmptyState(const char *title, const char *subtitle)
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        float fs = ImGui::GetFontSize();

        float wrap = std::max(80.0f, avail.x - 24.0f);
        ImFont *font = ImGui::GetFont();
        ImVec2 ts = font->CalcTextSizeA(fs, FLT_MAX, wrap, title);
        float y = origin.y + std::max(16.0f, avail.y * 0.35f - ts.y);
        float cx = origin.x + avail.x * 0.5f;

        dl->AddText(font, fs, ImVec2(std::floor(cx - ts.x * 0.5f), std::floor(y)), U32(Colors::TextDim), title, nullptr, wrap);
        if (subtitle)
        {
            ImVec2 ss = font->CalcTextSizeA(fs, FLT_MAX, wrap, subtitle);
            dl->AddText(font, fs, ImVec2(std::floor(cx - ss.x * 0.5f), std::floor(y + ts.y + 4.0f)), U32(Colors::TextFaint), subtitle, nullptr, wrap);
        }

        ImGui::Dummy(ImVec2(avail.x, std::max(avail.y, 60.0f)));
    }

    void VerticalSeparator(float height)
    {
        float h = height > 0.0f ? height : ImGui::GetFrameHeight();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x + 3.5f, pos.y + 3.0f), ImVec2(pos.x + 3.5f, pos.y + h - 3.0f), U32(Colors::Border));
        ImGui::Dummy(ImVec2(7.0f, h));
    }

    void IconText(const char *icon, const ImVec4 &iconColor, const char *text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
        ImGui::TextUnformatted(icon);
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 6.0f);
        ImGui::TextUnformatted(text);
    }

    bool IconMenuItem(const char *icon, const ImVec4 &iconColor, const char *label)
    {
        ImGuiStyle &style = ImGui::GetStyle();
        float fs = ImGui::GetFontSize();
        float gutter = std::floor(fs * 1.6f);
        float textW = ImGui::CalcTextSize(label).x;
        float width = gutter + textW + style.ItemSpacing.x * 2.0f;

        ImGui::PushID(label);
        bool pressed = ImGui::Selectable("##item", false, ImGuiSelectableFlags_SpanAvailWidth, ImVec2(width, 0.0f));
        ImGui::PopID();

        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        ImDrawList *dl = ImGui::GetWindowDrawList();
        float midY = (min.y + max.y) * 0.5f;

        DrawIcon(dl, icon, ImVec2(min.x + gutter * 0.5f, midY), U32(iconColor), fs);
        dl->AddText(ImVec2(min.x + gutter, std::floor(midY - fs * 0.5f)), ImGui::GetColorU32(ImGuiCol_Text), label);
        return pressed;
    }

    TreeRowResult TreeRow(const char *id, const char *icon, const ImVec4 &iconColor, const char *label, const TreeRowOptions &options)
    {
        TreeRowResult result;
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        ImGuiContext &g = *GImGui;

        ImGuiID gid = window->GetID(id);
        ImGuiStorage *storage = window->DC.StateStorage;
        bool open = !options.Leaf && (options.ForceOpen || storage->GetBool(gid, options.DefaultOpen));

        float rowH = Metrics::RowHeight;
        float arrowW = ImGui::GetStyle().IndentSpacing;
        ImVec2 pos = window->DC.CursorPos;
        float left = window->InnerRect.Min.x;
        float right = window->InnerRect.Max.x;
        ImRect bb(ImVec2(left, pos.y), ImVec2(right, pos.y + rowH));

        ImGui::ItemSize(ImVec2(std::max(1.0f, right - pos.x), rowH), 0.0f);
        if (ImGui::ItemAdd(bb, gid))
        {
            bool hovered = false, held = false;
            bool pressed = ImGui::ButtonBehavior(bb, gid, &hovered, &held, ImGuiButtonFlags_PressedOnClick);
            result.Hovered = hovered;

            ImRect arrowBb(ImVec2(pos.x - 2.0f, pos.y), ImVec2(pos.x + arrowW, pos.y + rowH));
            bool onArrow = !options.Leaf && arrowBb.Contains(g.IO.MousePos);

            if (pressed)
            {
                if (onArrow)
                    result.Toggled = true;
                else
                    result.Clicked = true;
            }
            if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !onArrow)
                result.DoubleClicked = true;
            if (result.Toggled && !options.ForceOpen)
            {
                open = !open;
                storage->SetBool(gid, open);
            }

            ImDrawList *dl = window->DrawList;
            if (options.Selected)
                dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Selection));
            else if (hovered)
                dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Hover));
            if (options.Highlight && !options.Selected)
                dl->AddRect(bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), U32(Colors::Accent, 0.6f));

            float cy = pos.y + rowH * 0.5f;
            float fontSize = ImGui::GetFontSize();
            if (!options.Leaf)
                DrawIcon(dl, open ? ICON_LC_CHEVRON_DOWN : ICON_LC_CHEVRON_RIGHT, ImVec2(pos.x + arrowW * 0.5f, cy), U32(Colors::TextFaint), fontSize * 0.8f);

            float x = pos.x + arrowW + 1.0f;
            if (icon)
            {
                DrawIcon(dl, icon, ImVec2(x + fontSize * 0.5f, cy), U32(iconColor), fontSize * 0.95f);
                x += fontSize + 6.0f;
            }

            float trailingW = 0.0f;
            if (options.Trailing)
            {
                ImVec2 tw = ImGui::CalcTextSize(options.Trailing);
                trailingW = tw.x + 10.0f;
                dl->AddText(ImVec2(right - tw.x - 8.0f, std::floor(cy - tw.y * 0.5f)), U32(Colors::TextFaint), options.Trailing);
            }

            result.LabelMin = ImVec2(x - 4.0f, pos.y + 1.0f);
            result.LabelMax = ImVec2(right - 4.0f - trailingW, pos.y + rowH - 1.0f);

            if (!options.HideLabel)
                DrawTextClipped(dl, ImGui::GetFont(), fontSize, ImVec2(x, std::floor(cy - fontSize * 0.5f)), right - 6.0f - trailingW, U32(Colors::Text), label);
        }

        result.Open = open;
        if (open)
        {
            ImGui::TreePushOverrideID(gid);
            ++s_TreeDepth;
        }
        return result;
    }

    void TreeRowPop()
    {
        ImGui::TreePop();
        if (s_TreeDepth > 0)
            --s_TreeDepth;
    }

    bool BeginSection(const char *id, const char *title, bool *removeRequested, bool defaultOpen)
    {
        ImGui::PushID(id);
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        ImGuiStorage *storage = window->DC.StateStorage;
        ImGuiID openId = window->GetID("##open");
        bool open = storage->GetBool(openId, defaultOpen);

        float h = ImGui::GetFrameHeight() + 2.0f;
        ImVec2 p = window->DC.CursorPos;
        float left = window->WorkRect.Min.x - window->WindowPadding.x * 0.5f;
        float right = window->WorkRect.Max.x + window->WindowPadding.x * 0.5f;
        float menuW = removeRequested ? h : 0.0f;

        ImRect headerBb(ImVec2(left, p.y), ImVec2(right - menuW, p.y + h));
        ImGuiID headerId = window->GetID("##header");
        ImGui::ItemSize(ImVec2(right - p.x, h), 0.0f);
        ImGui::ItemAdd(headerBb, headerId);

        bool hovered = false, held = false;
        if (ImGui::ButtonBehavior(headerBb, headerId, &hovered, &held))
        {
            open = !open;
            storage->SetBool(openId, open);
        }
        if (removeRequested)
            ImGui::OpenPopupOnItemClick("##sectionmenu", ImGuiPopupFlags_MouseButtonRight);

        ImDrawList *dl = window->DrawList;
        dl->AddRectFilled(ImVec2(left, p.y), ImVec2(right, p.y + h), U32(hovered ? Colors::HeaderHover : Colors::Header));

        float fontSize = ImGui::GetFontSize();
        float cy = p.y + h * 0.5f;
        DrawIcon(dl, open ? ICON_LC_CHEVRON_DOWN : ICON_LC_CHEVRON_RIGHT, ImVec2(left + 12.0f, cy), U32(Colors::TextDim), fontSize * 0.8f);
        dl->AddText(GetFont(FontRole::Medium), fontSize, ImVec2(left + 22.0f, std::floor(cy - fontSize * 0.5f)), U32(Colors::Text), title);

        if (removeRequested)
        {
            ImGui::SetCursorScreenPos(ImVec2(right - menuW, p.y + 1.0f));
            if (IconButton("##menu", ICON_LC_ELLIPSIS, nullptr, false, h - 2.0f))
                ImGui::OpenPopup("##sectionmenu");

            if (ImGui::BeginPopup("##sectionmenu"))
            {
                if (ImGui::MenuItem("Remove Component"))
                    *removeRequested = true;
                ImGui::EndPopup();
            }
        }

        if (open)
            ImGui::Dummy(ImVec2(0.0f, 2.0f));

        return open;
    }

    void EndSection()
    {
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0.0f, 3.0f));
    }

    void PropertyLabel(const char *label)
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        float labelW = LabelWidth();
        ImVec2 p = window->DC.CursorPos;
        float h = ImGui::GetFrameHeight();
        float fontSize = ImGui::GetFontSize();

        const char *end = ImGui::FindRenderedTextEnd(label);
        std::string text(label, end);
        DrawTextClipped(window->DrawList, ImGui::GetFont(), fontSize, ImVec2(p.x + 4.0f, std::floor(p.y + (h - fontSize) * 0.5f)), p.x + labelW - 6.0f, U32(Colors::TextDim), text.c_str());

        ImGui::SetCursorScreenPos(ImVec2(p.x + labelW, p.y));
        ImGui::SetNextItemWidth(-FLT_MIN);
    }

    bool PropertyFloat(const char *label, float *value, float speed, float min, float max, const char *format)
    {
        PropertyLabel(label);
        ImGui::PushID(label);
        bool changed = ImGui::DragFloat("##v", value, speed, min, max, format);
        ImGui::PopID();
        return Edited(changed);
    }

    bool PropertyInt(const char *label, int *value, float speed, int min, int max)
    {
        PropertyLabel(label);
        ImGui::PushID(label);
        bool changed = ImGui::DragInt("##v", value, speed, min, max);
        ImGui::PopID();
        return Edited(changed);
    }

    namespace
    {
        template <typename Fn>
        bool AxisPair(const char *label, Fn drag)
        {
            PropertyLabel(label);
            ImGui::PushID(label);

            static const char *axes[2] = {"x", "y"};
            const ImVec4 *axisColors[2] = {&Colors::AxisX, &Colors::AxisY};

            float spacing = 3.0f;
            float width = ImGui::GetContentRegionAvail().x;
            float half = std::floor((width - spacing) * 0.5f);
            float tagW = ImGui::CalcTextSize("x").x + 8.0f;
            bool changed = false;

            for (int i = 0; i < 2; ++i)
            {
                if (i > 0)
                    ImGui::SameLine(0.0f, spacing);

                ImGui::PushID(i);
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x + tagW * 0.5f, ImGui::GetStyle().FramePadding.y));
                ImGui::SetNextItemWidth(i == 0 ? half : width - half - spacing);
                changed |= drag(i);
                ImGui::PopStyleVar();

                ImGui::GetWindowDrawList()->AddText(ImVec2(pos.x + 5.0f, pos.y + ImGui::GetStyle().FramePadding.y), U32(*axisColors[i]), axes[i]);
                ImGui::PopID();
            }

            ImGui::PopID();
            return Edited(changed);
        }
    }

    bool PropertyVec2(const char *label, float *values, float speed, const char *format, float min, float max)
    {
        return AxisPair(label, [&](int i)
                        { return ImGui::DragFloat("##axis", &values[i], speed, min, max, format); });
    }

    bool PropertyInt2(const char *label, int *values, float speed, int min, int max)
    {
        return AxisPair(label, [&](int i)
                        { return ImGui::DragInt("##axis", &values[i], speed, min, max); });
    }

    bool PropertyBool(const char *label, bool *value)
    {
        PropertyLabel(label);
        ImGui::PushID(label);
        bool changed = ImGui::Checkbox("##v", value);
        ImGui::PopID();
        return Edited(changed);
    }

    bool PropertyBool2(const char *label, const char *a, bool *va, const char *b, bool *vb)
    {
        PropertyLabel(label);
        ImGui::PushID(label);
        bool changed = ImGui::Checkbox(a, va);
        ImGui::SameLine(0.0f, 12.0f);
        changed |= ImGui::Checkbox(b, vb);
        ImGui::PopID();
        return Edited(changed);
    }

    bool PropertyColor(const char *label, float *rgba)
    {
        PropertyLabel(label);
        ImGui::PushID(label);
        bool changed = ImGui::ColorEdit4("##v", rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
        ImGui::PopID();
        return Edited(changed);
    }

    bool PropertyColor(const char *label, SDL_Color &color)
    {
        float rgba[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        if (!PropertyColor(label, rgba))
            return false;
        color.r = static_cast<Uint8>(std::lround(rgba[0] * 255.0f));
        color.g = static_cast<Uint8>(std::lround(rgba[1] * 255.0f));
        color.b = static_cast<Uint8>(std::lround(rgba[2] * 255.0f));
        color.a = static_cast<Uint8>(std::lround(rgba[3] * 255.0f));
        return true;
    }

    bool PropertyText(const char *label, std::string &value, bool multiline)
    {
        PropertyLabel(label);
        ImGui::PushID(label);
        bool changed = multiline
                           ? ImGui::InputTextMultiline("##v", &value, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 3.0f + 8.0f))
                           : ImGui::InputText("##v", &value);
        ImGui::PopID();
        return Edited(changed);
    }

    void PropertyValue(const char *label, const char *value, const ImVec4 *color)
    {
        PropertyLabel(label);
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        ImVec2 p = window->DC.CursorPos;
        float h = ImGui::GetFrameHeight();
        float fontSize = ImGui::GetFontSize();
        float right = p.x + ImGui::GetContentRegionAvail().x;
        DrawTextClipped(window->DrawList, ImGui::GetFont(), fontSize, ImVec2(p.x + 2.0f, std::floor(p.y + (h - fontSize) * 0.5f)), right, U32(color ? *color : Colors::Text), value);
        ImGui::Dummy(ImVec2(right - p.x, h));
    }

    bool PropertySegmented(const char *label, int *current, const char *const *items, int count)
    {
        PropertyLabel(label);
        ImGui::PushID(label);
        bool changed = SegmentedControl("##v", current, items, count);
        ImGui::PopID();
        return Edited(changed);
    }

    bool SegmentedControl(const char *id, int *current, const char *const *items, int count, float width)
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems || count <= 0)
            return false;

        ImGui::PushID(id);
        float h = ImGui::GetFrameHeight();
        float w = width > 0.0f ? width : std::max(40.0f, ImGui::GetContentRegionAvail().x);

        ImVec2 pos = window->DC.CursorPos;
        ImRect frame(pos, pos + ImVec2(w, h));
        ImDrawList *dl = window->DrawList;
        dl->AddRectFilled(frame.Min, frame.Max, U32(Colors::Frame), Metrics::FrameRounding);

        float segW = w / static_cast<float>(count);
        bool changed = false;
        for (int i = 0; i < count; ++i)
        {
            ImRect seg(ImVec2(std::floor(pos.x + segW * i), pos.y), ImVec2(std::floor(pos.x + segW * (i + 1)), pos.y + h));
            ImGuiID sid = window->GetID(i);
            if (!ImGui::ItemAdd(seg, sid))
                continue;

            bool hovered = false, held = false;
            if (ImGui::ButtonBehavior(seg, sid, &hovered, &held) && *current != i)
            {
                *current = i;
                changed = true;
            }

            bool selected = (*current == i);
            if (selected)
                dl->AddRectFilled(seg.Min, seg.Max, U32(Colors::Selection), Metrics::FrameRounding);
            else if (hovered)
                dl->AddRectFilled(seg.Min, seg.Max, U32(Colors::Hover), Metrics::FrameRounding);
            if (i > 0 && !selected && *current != i - 1)
                dl->AddLine(ImVec2(seg.Min.x, seg.Min.y + 4.0f), ImVec2(seg.Min.x, seg.Max.y - 4.0f), U32(Colors::Border));

            const char *end = ImGui::FindRenderedTextEnd(items[i]);
            ImVec2 ts = ImGui::CalcTextSize(items[i], end);
            ImVec2 tp(std::floor(seg.GetCenter().x - ts.x * 0.5f), std::floor(seg.GetCenter().y - ts.y * 0.5f));
            dl->AddText(tp, U32(selected ? Colors::Text : Colors::TextDim), items[i], end);
        }

        ImGui::ItemSize(frame, ImGui::GetStyle().FramePadding.y);
        ImGui::PopID();
        return changed;
    }

    void DrawCheckerboard(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &max, float cell)
    {
        drawList->AddRectFilled(min, max, IM_COL32(52, 52, 52, 255));
        ImU32 light = IM_COL32(68, 68, 68, 255);
        for (float y = min.y; y < max.y; y += cell)
        {
            int row = static_cast<int>((y - min.y) / cell);
            for (float x = min.x + ((row & 1) ? cell : 0.0f); x < max.x; x += cell * 2.0f)
                drawList->AddRectFilled(ImVec2(x, y), ImVec2(std::min(x + cell, max.x), std::min(y + cell, max.y)), light);
        }
    }

    void DrawImageFit(ImDrawList *drawList, ImTextureID texture, const ImVec2 &textureSize, const ImVec2 &min, const ImVec2 &max, float rounding)
    {
        if (!texture || textureSize.x <= 0.0f || textureSize.y <= 0.0f)
            return;

        ImVec2 box = max - min;
        float scale = std::min(box.x / textureSize.x, box.y / textureSize.y);
        ImVec2 size(textureSize.x * scale, textureSize.y * scale);
        ImVec2 p0(std::floor(min.x + (box.x - size.x) * 0.5f), std::floor(min.y + (box.y - size.y) * 0.5f));
        drawList->AddImageRounded(texture, p0, p0 + size, ImVec2(0, 1), ImVec2(1, 0), IM_COL32_WHITE, rounding);
    }

    TileResult AssetTile(const char *id, ImTextureID texture, const ImVec2 &textureSize, const char *icon, const ImVec4 &color, const char *name, bool selected, float width)
    {
        TileResult result;
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return result;

        float fontSize = ImGui::GetFontSize();
        float pad = 4.0f;
        float thumb = width - pad * 2.0f;
        float height = pad + thumb + 4.0f + fontSize + pad;

        ImGuiID gid = window->GetID(id);
        ImVec2 pos = window->DC.CursorPos;
        ImRect bb(pos, pos + ImVec2(width, height));
        ImGui::ItemSize(bb, 0.0f);
        if (!ImGui::ItemAdd(bb, gid))
            return result;

        bool hovered = false, held = false;
        result.Clicked = ImGui::ButtonBehavior(bb, gid, &hovered, &held, ImGuiButtonFlags_PressedOnClick);
        result.Hovered = hovered;
        result.DoubleClicked = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        ImDrawList *dl = window->DrawList;
        if (selected)
            dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Selection), Metrics::FrameRounding);
        else if (hovered)
            dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Hover), Metrics::FrameRounding);

        ImVec2 tMin = bb.Min + ImVec2(pad, pad);
        ImVec2 tMax = tMin + ImVec2(thumb, thumb);
        if (texture)
        {
            DrawCheckerboard(dl, tMin, tMax, 6.0f);
            DrawImageFit(dl, texture, textureSize, tMin + ImVec2(3, 3), tMax - ImVec2(3, 3));
        }
        else
        {
            dl->AddRectFilled(tMin, tMax, U32(Colors::Frame), Metrics::FrameRounding);
            DrawIcon(dl, icon, (tMin + tMax) * 0.5f, U32(color), std::floor(thumb * 0.36f));
        }

        float textY = tMax.y + 4.0f;
        ImFont *font = ImGui::GetFont();
        ImVec2 ns = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, name);
        float textX = std::max(bb.Min.x + pad, std::floor(bb.GetCenter().x - ns.x * 0.5f));
        DrawTextClipped(dl, font, fontSize, ImVec2(textX, textY), bb.Max.x - pad, U32(Colors::Text), name);

        return result;
    }

    bool AssetRow(const char *id, ImTextureID texture, const ImVec2 &textureSize, const char *icon, const ImVec4 &color, const char *name, const char *type, bool selected)
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        float rowH = Metrics::RowHeight;
        ImGuiID gid = window->GetID(id);
        ImVec2 pos = window->DC.CursorPos;
        float left = window->InnerRect.Min.x;
        float right = window->InnerRect.Max.x;
        ImRect bb(ImVec2(left, pos.y), ImVec2(right, pos.y + rowH));
        ImGui::ItemSize(ImVec2(right - pos.x, rowH), 0.0f);
        if (!ImGui::ItemAdd(bb, gid))
            return false;

        bool hovered = false, held = false;
        bool pressed = ImGui::ButtonBehavior(bb, gid, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

        ImDrawList *dl = window->DrawList;
        if (selected)
            dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Selection));
        else if (hovered)
            dl->AddRectFilled(bb.Min, bb.Max, U32(Colors::Hover));

        float fontSize = ImGui::GetFontSize();
        float cy = pos.y + rowH * 0.5f;
        ImVec2 iMin(pos.x + 2.0f, cy - 8.0f);
        ImVec2 iMax(iMin.x + 16.0f, cy + 8.0f);
        if (texture)
            DrawImageFit(dl, texture, textureSize, iMin, iMax);
        else
            DrawIcon(dl, icon, (iMin + iMax) * 0.5f, U32(color), fontSize * 0.95f);

        float typeW = 0.0f;
        if (type)
        {
            ImVec2 ts = ImGui::CalcTextSize(type);
            typeW = ts.x + 16.0f;
            dl->AddText(ImVec2(right - ts.x - 8.0f, std::floor(cy - ts.y * 0.5f)), U32(Colors::TextFaint), type);
        }
        DrawTextClipped(dl, ImGui::GetFont(), fontSize, ImVec2(iMax.x + 8.0f, std::floor(cy - fontSize * 0.5f)), right - typeW, U32(Colors::Text), name);
        return pressed;
    }

    bool Breadcrumbs(const std::string &virtualPath, std::string &outClicked)
    {
        size_t schemeEnd = virtualPath.find("://");
        std::string root = schemeEnd == std::string::npos ? virtualPath : virtualPath.substr(0, schemeEnd + 3);
        std::string rest = schemeEnd == std::string::npos ? std::string() : virtualPath.substr(schemeEnd + 3);

        std::vector<std::string> parts;
        size_t start = 0;
        while (start < rest.size())
        {
            size_t slash = rest.find('/', start);
            if (slash == std::string::npos)
                slash = rest.size();
            if (slash > start)
                parts.push_back(rest.substr(start, slash - start));
            start = slash + 1;
        }

        bool clicked = false;
        ImGui::PushID("##crumbs");
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, ImGui::GetStyle().FramePadding.y));

        std::string rootLabel = root.substr(0, root.find("://"));
        if (Button(rootLabel.c_str(), ButtonKind::Ghost))
        {
            outClicked = root;
            clicked = true;
        }

        std::string accum = root;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            ImGui::SameLine(0.0f, 0.0f);
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
            ImGui::TextUnformatted("/");
            ImGui::PopStyleColor();
            ImGui::SameLine(0.0f, 0.0f);

            accum += (i == 0 ? "" : "/") + parts[i];
            ImGui::PushID(static_cast<int>(i));
            if (Button(parts[i].c_str(), ButtonKind::Ghost))
            {
                outClicked = accum;
                clicked = true;
            }
            ImGui::PopID();
        }

        ImGui::PopStyleVar();
        ImGui::PopID();
        return clicked;
    }

    bool BeginModal(const char *title, bool *open, const ImVec2 &size)
    {
        if (open && *open && !ImGui::IsPopupOpen(title))
            ImGui::OpenPopup(title);

        if (size.x > 0.0f)
            ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, Colors::Header);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, Colors::Header);
        ImGui::PushStyleColor(ImGuiCol_Border, Colors::BorderStrong);

        ImGuiWindowFlags flags = size.x > 0.0f ? ImGuiWindowFlags_None : ImGuiWindowFlags_AlwaysAutoResize;
        bool visible = ImGui::BeginPopupModal(title, open, flags | ImGuiWindowFlags_NoSavedSettings);

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        return visible;
    }

    void EndModal()
    {
        ImGui::EndPopup();
    }

    void BeginToolbar(const char *id, float height)
    {
        float h = height > 0.0f ? height : ImGui::GetFrameHeight() + 6.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Header);
        ImGui::BeginChild(id, ImVec2(0, h), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleColor();
    }

    void EndToolbar()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
    }
}
