#pragma once

#include "Theme.h"
#include "IconsLucide.h"

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Scene.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>

namespace EditorUI
{
    struct Visual
    {
        const char *Icon;
        const ImVec4 *Tint;
        const char *Type;

        const ImVec4 &Color() const { return *Tint; }
    };

    inline std::filesystem::path Utf8Path(const std::string &utf8)
    {
        return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t *>(utf8.data()), utf8.size()));
    }

    inline std::string LowerExtension(const std::filesystem::path &path)
    {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return ext;
    }

    inline bool IsImageExtension(const std::string &ext)
    {
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".webp" || ext == ".gif" || ext == ".tga";
    }

    inline bool IsSceneExtension(const std::string &ext)
    {
        return ext == ".yaml" || ext == ".yml" || ext == ".simpscene";
    }

    inline Visual FileVisual(const std::filesystem::path &path, bool isDirectory)
    {
        if (isDirectory)
            return {ICON_LC_FOLDER, &Colors::FolderTint, "Folder"};

        std::string ext = LowerExtension(path);
        if (IsImageExtension(ext))
            return {ICON_LC_IMAGE, &Colors::TextDim, "Texture"};
        if (ext == ".svg")
            return {ICON_LC_SHAPES, &Colors::TextDim, "Vector"};
        if (IsSceneExtension(ext))
            return {ICON_LC_CLAPPERBOARD, &Colors::NodeTint, "Scene"};
        if (ext == ".json")
            return {ICON_LC_BRACES, &Colors::TextDim, "JSON"};
        if (ext == ".prefab")
            return {ICON_LC_PACKAGE, &Colors::NodeTint, "Prefab"};
        if (ext == ".particle")
            return {ICON_LC_SPARKLES, &Colors::NodeTint, "Particle Effect"};
        if (ext == ".ttf" || ext == ".otf")
            return {ICON_LC_TYPE, &Colors::TextDim, "Font"};
        if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac")
            return {ICON_LC_FILE_AUDIO, &Colors::TextDim, "Audio"};
        if (ext == ".tiled-session" || ext == ".tmx" || ext == ".tsx")
            return {ICON_LC_MAP, &Colors::TextDim, "Tiled"};
        if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".lua" || ext == ".glsl")
            return {ICON_LC_FILE_CODE, &Colors::TextDim, "Code"};
        if (ext == ".txt" || ext == ".md")
            return {ICON_LC_FILE_TEXT, &Colors::TextDim, "Text"};
        return {ICON_LC_FILE, &Colors::TextFaint, "File"};
    }

    inline Visual EntityVisual(SIMPEngine::Entity entity)
    {
        if (entity.HasComponent<CameraComponent>())
            return {ICON_LC_VIDEO, &Colors::TintCamera, "Camera"};
        if (entity.HasComponent<TilemapComponent>())
            return {ICON_LC_GRID_3X3, &Colors::TintVisual, "Tilemap"};
        if (entity.HasComponent<LightComponent>())
            return {ICON_LC_LIGHTBULB, &Colors::TintLight, "Light"};
        if (entity.HasComponent<MusicComponent>())
            return {ICON_LC_MUSIC, &Colors::TintAudio, "Music"};
        if (entity.HasComponent<AudioSourceComponent>())
            return {ICON_LC_VOLUME_2, &Colors::TintAudio, "Audio"};
        if (entity.HasComponent<ParticleEmitterComponent>())
            return {ICON_LC_SPARKLES, &Colors::TintLight, "Particles"};
        if (entity.HasComponent<AnimatedSpriteComponent>())
            return {ICON_LC_FILM, &Colors::TintVisual, "Animated Sprite"};
        if (entity.HasComponent<SpriteComponent>())
            return {ICON_LC_IMAGE, &Colors::TintVisual, "Sprite"};
        if (entity.HasComponent<VectorShapeComponent>())
            return {ICON_LC_PEN_TOOL, &Colors::TintVisual, "Vector Shape"};
        if (entity.HasComponent<TextComponent>())
            return {ICON_LC_TYPE, &Colors::TintVisual, "Text"};
        if (entity.HasComponent<PhysicsComponent>())
            return {ICON_LC_ATOM, &Colors::TintPhysics, "Body"};
        if (entity.HasComponent<RenderComponent>())
            return {ICON_LC_SQUARE, &Colors::TintVisual, "Shape"};
        if (entity.HasComponent<ShaderCanvasComponent>())
            return {ICON_LC_WAND_SPARKLES, &Colors::TintLight, "Shader Canvas"};
        if (entity.HasComponent<CanvasComponent>())
            return {ICON_LC_MONITOR, &Colors::TintUI, "Canvas"};
        if (entity.HasComponent<UIButtonComponent>())
            return {ICON_LC_SQUARE_MOUSE_POINTER, &Colors::TintUI, "UI Button"};
        if (entity.HasComponent<UILabelComponent>())
            return {ICON_LC_TYPE, &Colors::TintUI, "UI Label"};
        if (entity.HasComponent<UIImageComponent>())
            return {ICON_LC_IMAGE, &Colors::TintUI, "UI Image"};
        if (entity.HasComponent<UILayoutComponent>())
            return {ICON_LC_LAYOUT_LIST, &Colors::TintUI, "UI Layout"};
        if (entity.HasComponent<UIPanelComponent>())
            return {ICON_LC_PANEL_TOP, &Colors::TintUI, "UI Panel"};
        if (entity.HasComponent<UIRectComponent>())
            return {ICON_LC_RECTANGLE_HORIZONTAL, &Colors::TintUI, "UI Node"};
        if (entity.HasComponent<ButtonComponent>())
            return {ICON_LC_MOUSE_POINTER_CLICK, &Colors::TintLogic, "Button"};
        if (entity.HasComponent<TeleporterComponent>())
            return {ICON_LC_DOOR_OPEN, &Colors::TintLogic, "Teleporter"};
        if (entity.HasComponent<CameraZoneComponent>())
            return {ICON_LC_SCAN_EYE, &Colors::TintCamera, "Camera Zone"};
        if (entity.HasComponent<PickableComponent>())
            return {ICON_LC_GRAB, &Colors::TintLogic, "Pickable"};
        if (entity.HasComponent<CheckpointComponent>())
            return {ICON_LC_FLAG, &Colors::TintLogic, "Checkpoint"};
        if (entity.HasComponent<PickupComponent>())
            return {ICON_LC_GEM, &Colors::TintLogic, "Pickup"};
        if (entity.HasComponent<SpawnerComponent>())
            return {ICON_LC_COPY_PLUS, &Colors::TintLogic, "Spawner"};
        if (entity.HasComponent<DamageComponent>())
            return {ICON_LC_SWORDS, &Colors::TintLogic, "Damage"};
        if (entity.HasComponent<TriggerComponent>())
            return {ICON_LC_RADAR, &Colors::TintPhysics, "Trigger"};
        if (entity.HasComponent<CollisionComponent>())
            return {ICON_LC_SCAN, &Colors::TintPhysics, "Area"};
        return {ICON_LC_CIRCLE_DASHED, &Colors::TextDim, "Entity"};
    }

    inline Visual ComponentVisual(const char *title)
    {
        struct Entry
        {
            const char *Title;
            const char *Icon;
            const ImVec4 *Tint;
        };
        static const Entry entries[] = {
            {"Transform", ICON_LC_MOVE, &Colors::TextDim},
            {"Hierarchy", ICON_LC_LIST_TREE, &Colors::TextDim},
            {"Prefab", ICON_LC_PACKAGE, &Colors::TextDim},
            {"Velocity", ICON_LC_WIND, &Colors::TintPhysics},
            {"Camera", ICON_LC_VIDEO, &Colors::TintCamera},
            {"Camera Zone", ICON_LC_SCAN_EYE, &Colors::TintCamera},
            {"Shape", ICON_LC_SQUARE, &Colors::TintVisual},
            {"Sprite", ICON_LC_IMAGE, &Colors::TintVisual},
            {"Animated Sprite", ICON_LC_FILM, &Colors::TintVisual},
            {"Vector Shape", ICON_LC_PEN_TOOL, &Colors::TintVisual},
            {"Text", ICON_LC_TYPE, &Colors::TintVisual},
            {"Tilemap", ICON_LC_GRID_3X3, &Colors::TintVisual},
            {"Clip Area", ICON_LC_SCAN, &Colors::TintVisual},
            {"Parallax", ICON_LC_LAYERS, &Colors::TintVisual},
            {"Particle Emitter", ICON_LC_SPARKLES, &Colors::TintLight},
            {"Light", ICON_LC_LIGHTBULB, &Colors::TintLight},
            {"Glow", ICON_LC_SPARKLE, &Colors::TintLight},
            {"Shader Canvas", ICON_LC_WAND_SPARKLES, &Colors::TintLight},
            {"Audio Source", ICON_LC_VOLUME_2, &Colors::TintAudio},
            {"Music", ICON_LC_MUSIC, &Colors::TintAudio},
            {"Collision", ICON_LC_SCAN, &Colors::TintPhysics},
            {"Rigid Body", ICON_LC_ATOM, &Colors::TintPhysics},
            {"Character Controller", ICON_LC_GAMEPAD_2, &Colors::TintPhysics},
            {"Trigger Zone", ICON_LC_RADAR, &Colors::TintPhysics},
            {"World Bounds", ICON_LC_SQUARE_DASHED, &Colors::TintPhysics},
            {"Canvas", ICON_LC_MONITOR, &Colors::TintUI},
            {"UI Rect", ICON_LC_RECTANGLE_HORIZONTAL, &Colors::TintUI},
            {"UI Panel", ICON_LC_PANEL_TOP, &Colors::TintUI},
            {"UI Label", ICON_LC_TYPE, &Colors::TintUI},
            {"UI Image", ICON_LC_IMAGE, &Colors::TintUI},
            {"UI Button", ICON_LC_SQUARE_MOUSE_POINTER, &Colors::TintUI},
            {"UI Layout", ICON_LC_LAYOUT_LIST, &Colors::TintUI},
            {"Script", ICON_LC_CODE, &Colors::TintLogic},
            {"Timer", ICON_LC_TIMER, &Colors::TintLogic},
            {"Lifetime", ICON_LC_HOURGLASS, &Colors::TintLogic},
            {"Path Follower", ICON_LC_ROUTE, &Colors::TintLogic},
            {"Health", ICON_LC_HEART, &Colors::TintLogic},
            {"Damage", ICON_LC_SWORDS, &Colors::TintLogic},
            {"Button", ICON_LC_MOUSE_POINTER_CLICK, &Colors::TintLogic},
            {"Teleporter", ICON_LC_DOOR_OPEN, &Colors::TintLogic},
            {"Pickable", ICON_LC_GRAB, &Colors::TintLogic},
            {"Follow", ICON_LC_CROSSHAIR, &Colors::TintLogic},
            {"Pickup", ICON_LC_GEM, &Colors::TintLogic},
            {"Checkpoint", ICON_LC_FLAG, &Colors::TintLogic},
            {"Respawn", ICON_LC_ROTATE_CCW, &Colors::TintLogic},
            {"Oscillator", ICON_LC_ACTIVITY, &Colors::TintLogic},
            {"Spawner", ICON_LC_COPY_PLUS, &Colors::TintLogic},
        };
        for (const Entry &entry : entries)
            if (std::string_view(entry.Title) == title)
                return {entry.Icon, entry.Tint, entry.Title};
        return {nullptr, &Colors::TextDim, title};
    }
}
