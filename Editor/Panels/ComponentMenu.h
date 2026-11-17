#pragma once

#include "UI/Visuals.h"
#include "UI/Widgets.h"
#include "../EditorCommands.h"

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

struct ComponentEntry
{
    const char *Category;
    const char *Icon;
    const char *Name;
    const char *Keywords;
    bool (*Has)(SIMPEngine::Entity &);
    void (*Add)(SIMPEngine::Entity &);
};

namespace ComponentMenu
{
    template <typename T>
    bool Has(SIMPEngine::Entity &e) { return e.HasComponent<T>(); }

    template <typename T>
    void Add(SIMPEngine::Entity &e) { e.AddComponent<T>(); }

    template <typename T>
    void AddWithRect(SIMPEngine::Entity &e)
    {
        e.AddComponent<T>();
        if (!e.HasComponent<UIRectComponent>())
            e.AddComponent<UIRectComponent>();
    }

    template <typename T, bool PlayerOnly>
    void AddWithTrigger(SIMPEngine::Entity &e)
    {
        e.AddComponent<T>();
        if (!e.HasComponent<TriggerComponent>())
        {
            auto &zone = e.AddComponent<TriggerComponent>();
            if (PlayerOnly)
                zone.filter = "Player";
        }
    }

    inline void AddShape(SIMPEngine::Entity &e)
    {
        auto &r = e.AddComponent<RenderComponent>();
        r.width = 64.0f;
        r.height = 64.0f;
    }

    inline void AddSprite(SIMPEngine::Entity &e)
    {
        auto &s = e.AddComponent<SpriteComponent>();
        s.width = 64.0f;
        s.height = 64.0f;
    }

    inline void AddController(SIMPEngine::Entity &e)
    {
        auto &controller = e.AddComponent<CharacterControllerComponent>();
        if (e.HasComponent<SpriteComponent>())
        {
            const auto &sprite = e.GetComponent<SpriteComponent>();
            const glm::vec2 scale = glm::abs(e.GetComponent<TransformComponent>().scale);
            if (sprite.width > 0.0f && sprite.height > 0.0f)
                controller.size = glm::vec2(sprite.width, sprite.height) * scale;
        }
    }

    inline const std::vector<ComponentEntry> &Entries()
    {
        static const std::vector<ComponentEntry> entries = {
            {"Spatial", ICON_LC_MOVE, "Transform", "position rotation scale", &Has<TransformComponent>, &Add<TransformComponent>},
            {"Spatial", ICON_LC_WIND, "Velocity", "speed move motion", &Has<VelocityComponent>, &Add<VelocityComponent>},
            {"Spatial", ICON_LC_VIDEO, "Camera", "view zoom follow", &Has<CameraComponent>, &Add<CameraComponent>},
            {"Rendering", ICON_LC_SQUARE, "Shape", "rectangle quad color box", &Has<RenderComponent>, &AddShape},
            {"Rendering", ICON_LC_IMAGE, "Sprite", "image texture picture png", &Has<SpriteComponent>, &AddSprite},
            {"Rendering", ICON_LC_FILM, "Animated Sprite", "animation frames sheet", &Has<AnimatedSpriteComponent>, &Add<AnimatedSpriteComponent>},
            {"Rendering", ICON_LC_PEN_TOOL, "Vector Shape", "svg path circle line polygon", &Has<VectorShapeComponent>, &Add<VectorShapeComponent>},
            {"Rendering", ICON_LC_TYPE, "Text", "label font words", &Has<TextComponent>, &Add<TextComponent>},
            {"Rendering", ICON_LC_GRID_3X3, "Tilemap", "tiles level grid", &Has<TilemapComponent>, &Add<TilemapComponent>},
            {"Rendering", ICON_LC_SPARKLES, "Particle Emitter", "particles fx smoke fire sparks", &Has<ParticleEmitterComponent>, &Add<ParticleEmitterComponent>},
            {"Rendering", ICON_LC_LIGHTBULB, "Light", "lamp point spot lighting", &Has<LightComponent>, &Add<LightComponent>},
            {"Rendering", ICON_LC_SPARKLE, "Glow", "bloom emissive neon", &Has<GlowComponent>, &Add<GlowComponent>},
            {"Rendering", ICON_LC_SCAN, "Clip Area", "mask scissor scroll crop clip", &Has<ClipComponent>, &Add<ClipComponent>},
            {"Rendering", ICON_LC_LAYERS, "Parallax", "background depth layers", &Has<ParallaxComponent>, &Add<ParallaxComponent>},
            {"Rendering", ICON_LC_WAND_SPARKLES, "Shader Canvas", "shader post effect glsl", &Has<ShaderCanvasComponent>, &Add<ShaderCanvasComponent>},
            {"Audio", ICON_LC_VOLUME_2, "Audio Source", "sound sfx play", &Has<AudioSourceComponent>, &Add<AudioSourceComponent>},
            {"Audio", ICON_LC_MUSIC, "Music", "song bgm soundtrack", &Has<MusicComponent>, &Add<MusicComponent>},
            {"Physics", ICON_LC_SCAN, "Collision", "collider hitbox solid box", &Has<CollisionComponent>, &Add<CollisionComponent>},
            {"Physics", ICON_LC_ATOM, "Rigid Body", "physics body gravity dynamic", &Has<PhysicsComponent>, &Add<PhysicsComponent>},
            {"Physics", ICON_LC_GAMEPAD_2, "Character Controller", "player platformer move jump walk", &Has<CharacterControllerComponent>, &AddController},
            {"Physics", ICON_LC_RADAR, "Trigger Zone", "area sensor overlap enter", &Has<TriggerComponent>, &Add<TriggerComponent>},
            {"Physics", ICON_LC_SQUARE_DASHED, "World Bounds", "clamp wrap edge limits", &Has<WorldBoundsComponent>, &Add<WorldBoundsComponent>},
            {"Logic", ICON_LC_CODE, "Script", "lua code behaviour", &Has<ScriptComponent>, &Add<ScriptComponent>},
            {"Logic", ICON_LC_TIMER, "Timer", "delay countdown repeat", &Has<TimerComponent>, &Add<TimerComponent>},
            {"Logic", ICON_LC_HOURGLASS, "Lifetime", "destroy timeout expire", &Has<LifetimeComponent>, &Add<LifetimeComponent>},
            {"Logic", ICON_LC_ROUTE, "Path Follower", "waypoints route patrol", &Has<PathFollowerComponent>, &Add<PathFollowerComponent>},
            {"Logic", ICON_LC_HEART, "Health", "hp life damage", &Has<HealthComponent>, &Add<HealthComponent>},
            {"Logic", ICON_LC_SWORDS, "Damage", "hurt hazard spike enemy", &Has<DamageComponent>, &AddWithTrigger<DamageComponent, false>},
            {"Logic", ICON_LC_GEM, "Pickup", "collect coin item", &Has<PickupComponent>, &AddWithTrigger<PickupComponent, true>},
            {"Logic", ICON_LC_FLAG, "Checkpoint", "save spawn point", &Has<CheckpointComponent>, &AddWithTrigger<CheckpointComponent, false>},
            {"Logic", ICON_LC_ROTATE_CCW, "Respawn", "death revive", &Has<RespawnComponent>, &Add<RespawnComponent>},
            {"Logic", ICON_LC_DOOR_OPEN, "Teleporter", "portal warp door", &Has<TeleporterComponent>, &AddWithTrigger<TeleporterComponent, true>},
            {"Logic", ICON_LC_SCAN_EYE, "Camera Zone", "camera area room", &Has<CameraZoneComponent>, &AddWithTrigger<CameraZoneComponent, true>},
            {"Logic", ICON_LC_COPY_PLUS, "Spawner", "clone spawn instantiate", &Has<SpawnerComponent>, &Add<SpawnerComponent>},
            {"Logic", ICON_LC_ACTIVITY, "Oscillator", "bob sway wave float", &Has<OscillatorComponent>, &Add<OscillatorComponent>},
            {"Logic", ICON_LC_CROSSHAIR, "Follow", "chase track target", &Has<FollowComponent>, &Add<FollowComponent>},
            {"Logic", ICON_LC_GRAB, "Pickable", "drag grab mouse throw", &Has<PickableComponent>, &Add<PickableComponent>},
            {"Logic", ICON_LC_MOUSE_POINTER_CLICK, "Button", "click press hover", &Has<ButtonComponent>, &Add<ButtonComponent>},
            {"UI", ICON_LC_MONITOR, "Canvas", "ui root hud screen", &Has<CanvasComponent>, &Add<CanvasComponent>},
            {"UI", ICON_LC_RECTANGLE_HORIZONTAL, "UI Rect", "layout anchor", &Has<UIRectComponent>, &Add<UIRectComponent>},
            {"UI", ICON_LC_PANEL_TOP, "UI Panel", "background box", &Has<UIPanelComponent>, &AddWithRect<UIPanelComponent>},
            {"UI", ICON_LC_TYPE, "UI Label", "text", &Has<UILabelComponent>, &AddWithRect<UILabelComponent>},
            {"UI", ICON_LC_IMAGE, "UI Image", "picture icon", &Has<UIImageComponent>, &AddWithRect<UIImageComponent>},
            {"UI", ICON_LC_SQUARE_MOUSE_POINTER, "UI Button", "click menu", &Has<UIButtonComponent>, &AddWithRect<UIButtonComponent>},
            {"UI", ICON_LC_LAYOUT_LIST, "UI Layout", "row column stack", &Has<UILayoutComponent>, &AddWithRect<UILayoutComponent>},
        };
        return entries;
    }

    inline int KeywordScore(const char *keywords, const char *query)
    {
        const std::string_view words(keywords);
        std::string lowered(query);
        for (char &c : lowered)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        int best = -1;
        size_t start = 0;
        while (start < words.size())
        {
            size_t end = words.find(' ', start);
            if (end == std::string_view::npos)
                end = words.size();
            const std::string_view word = words.substr(start, end - start);
            if (word.substr(0, lowered.size()) == lowered)
                best = std::max(best, 120);
            else if (lowered.size() >= 3 && word.find(lowered) != std::string_view::npos)
                best = std::max(best, 50);
            start = end + 1;
        }
        return best;
    }

    inline int Score(const ComponentEntry &entry, const char *query)
    {
        return std::max(EditorUI::FuzzyScore(entry.Name, query), KeywordScore(entry.Keywords, query));
    }

    inline std::vector<const ComponentEntry *> Ranked(SIMPEngine::Entity entity, const char *query)
    {
        std::vector<std::pair<int, const ComponentEntry *>> scored;
        for (const ComponentEntry &entry : Entries())
        {
            if (entry.Has(entity))
                continue;
            const int score = Score(entry, query);
            if (score >= 0)
                scored.emplace_back(score, &entry);
        }
        std::stable_sort(scored.begin(), scored.end(), [](const auto &a, const auto &b) { return a.first > b.first; });
        std::vector<const ComponentEntry *> out;
        for (const auto &item : scored)
            out.push_back(item.second);
        return out;
    }

    inline void Apply(EditorContext &ctx, SIMPEngine::Entity entity, const ComponentEntry &entry)
    {
        EditorCommands::RecordEdit(ctx);
        entry.Add(entity);
        ctx.Scene->MarkPhysicsDirty();
    }

    inline bool Item(EditorContext &ctx, SIMPEngine::Entity entity, const ComponentEntry &entry, bool highlighted, const char *hint)
    {
        const EditorUI::Visual visual = EditorUI::ComponentVisual(entry.Name);
        if (!EditorUI::IconMenuItem(entry.Icon, visual.Tint ? *visual.Tint : EditorUI::Colors::NodeTint, entry.Name, highlighted, hint))
            return false;
        Apply(ctx, entity, entry);
        return true;
    }
}

inline bool DrawAddComponentItems(EditorContext &ctx, SIMPEngine::Entity entity, const char *filter = nullptr, int *highlight = nullptr)
{
    if (filter && *filter)
    {
        const std::vector<const ComponentEntry *> ranked = ComponentMenu::Ranked(entity, filter);
        if (ranked.empty())
        {
            ImGui::TextDisabled("No component matches \"%s\"", filter);
            return false;
        }
        int selected = highlight ? std::clamp(*highlight, 0, static_cast<int>(ranked.size()) - 1) : -1;
        bool moved = false;
        if (highlight)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            {
                selected = std::min(selected + 1, static_cast<int>(ranked.size()) - 1);
                moved = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            {
                selected = std::max(selected - 1, 0);
                moved = true;
            }
            *highlight = selected;
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
            {
                ComponentMenu::Apply(ctx, entity, *ranked[static_cast<size_t>(selected)]);
                return true;
            }
        }
        for (size_t i = 0; i < ranked.size(); ++i)
        {
            if (ComponentMenu::Item(ctx, entity, *ranked[i], static_cast<int>(i) == selected, ranked[i]->Category))
                return true;
            if (moved && static_cast<int>(i) == selected)
                ImGui::SetScrollHereY();
        }
        if (highlight)
            ImGui::TextDisabled("Enter adds the highlighted one");
        return false;
    }

    const char *category = nullptr;
    bool any = false;
    for (const ComponentEntry &entry : ComponentMenu::Entries())
    {
        if (entry.Has(entity))
            continue;
        if (!category || std::string_view(category) != entry.Category)
        {
            category = entry.Category;
            EditorUI::SectionLabel(category);
        }
        any = true;
        if (ComponentMenu::Item(ctx, entity, entry, false, nullptr))
            return true;
    }
    if (!any)
        ImGui::TextDisabled("Nothing to add");
    return false;
}
