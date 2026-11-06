#pragma once

#include "Theme.h"
#include "IconsLucide.h"

#include <imgui.h>
#include <SDL3/SDL_pixels.h>
#include <cfloat>
#include <cstdint>
#include <type_traits>
#include <cstddef>
#include <functional>
#include <string>

namespace EditorUI
{
    enum class ButtonKind
    {
        Primary,
        Secondary,
        Ghost,
        Danger
    };

    template <typename T>
    inline ImTextureID TextureId(T id)
    {
        if constexpr (std::is_pointer_v<ImTextureID>)
            return reinterpret_cast<ImTextureID>(static_cast<std::uintptr_t>(id));
        else
            return static_cast<ImTextureID>(id);
    }

    void SetEditHook(std::function<void()> hook);
    void NotifyEdit();
    void PushEditSuppression();
    void PopEditSuppression();

    void Tooltip(const char *text);
    bool ContainsInsensitive(const char *haystack, const char *needle);

    bool IconButton(const char *id, const char *icon, const char *tooltip = nullptr, bool active = false, float size = 0.0f);
    bool Button(const char *label, ButtonKind kind = ButtonKind::Secondary, const ImVec2 &size = ImVec2(0, 0));
    bool SearchBox(const char *id, char *buffer, size_t bufferSize, const char *hint = "Search", float width = -FLT_MIN);

    void SectionLabel(const char *text);
    void EmptyState(const char *title, const char *subtitle = nullptr);
    void VerticalSeparator(float height = 0.0f);
    void IconText(const char *icon, const ImVec4 &iconColor, const char *text);
    bool IconMenuItem(const char *icon, const ImVec4 &iconColor, const char *label);

    struct TreeRowOptions
    {
        bool Selected = false;
        bool Leaf = false;
        bool DefaultOpen = false;
        bool ForceOpen = false;
        bool Highlight = false;
        bool HideLabel = false;
        const char *Trailing = nullptr;
    };

    struct TreeRowResult
    {
        bool Open = false;
        bool Clicked = false;
        bool DoubleClicked = false;
        bool Hovered = false;
        bool Toggled = false;
        ImVec2 LabelMin;
        ImVec2 LabelMax;
    };

    TreeRowResult TreeRow(const char *id, const char *icon, const ImVec4 &iconColor, const char *label, const TreeRowOptions &options = {});
    void TreeRowPop();

    bool BeginSection(const char *id, const char *title, bool *removeRequested = nullptr, bool defaultOpen = true);
    void EndSection();

    void PropertyLabel(const char *label);
    bool PropertyFloat(const char *label, float *value, float speed = 0.1f, float min = 0.0f, float max = 0.0f, const char *format = "%.2f");
    bool PropertyInt(const char *label, int *value, float speed = 1.0f, int min = 0, int max = 0);
    bool PropertyVec2(const char *label, float *values, float speed = 0.1f, const char *format = "%.2f", float min = 0.0f, float max = 0.0f);
    bool PropertyRange(const char *label, float *values, float speed = 0.1f, const char *format = "%.2f", float min = 0.0f, float max = 0.0f);
    bool PropertyInt2(const char *label, int *values, float speed = 1.0f, int min = 0, int max = 0);
    bool PropertyBool(const char *label, bool *value);
    bool PropertyBool2(const char *label, const char *a, bool *va, const char *b, bool *vb);
    bool PropertyColor(const char *label, float *rgba);
    bool PropertyColor(const char *label, SDL_Color &color);
    bool PropertyText(const char *label, std::string &value, bool multiline = false);
    void PropertyValue(const char *label, const char *value, const ImVec4 *color = nullptr);
    bool PropertySegmented(const char *label, int *current, const char *const *items, int count);

    bool SegmentedControl(const char *id, int *current, const char *const *items, int count, float width = -FLT_MIN);

    struct TileResult
    {
        bool Clicked = false;
        bool DoubleClicked = false;
        bool Hovered = false;
    };

    TileResult AssetTile(const char *id, ImTextureID texture, const ImVec2 &textureSize, const char *icon, const ImVec4 &color, const char *name, bool selected, float width);
    bool AssetRow(const char *id, ImTextureID texture, const ImVec2 &textureSize, const char *icon, const ImVec4 &color, const char *name, const char *type, bool selected);

    bool Breadcrumbs(const std::string &virtualPath, std::string &outClicked);

    void DrawCheckerboard(ImDrawList *drawList, const ImVec2 &min, const ImVec2 &max, float cell = 8.0f);
    void DrawImageFit(ImDrawList *drawList, ImTextureID texture, const ImVec2 &textureSize, const ImVec2 &min, const ImVec2 &max, float rounding = 0.0f);

    bool BeginModal(const char *title, bool *open, const ImVec2 &size = ImVec2(0, 0));
    void EndModal();

    void BeginToolbar(const char *id, float height = 0.0f);
    void EndToolbar();
}
