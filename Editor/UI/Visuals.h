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
            return {ICON_LC_VIDEO, &Colors::NodeTint, "Camera"};
        if (entity.HasComponent<TilemapComponent>())
            return {ICON_LC_GRID_3X3, &Colors::NodeTint, "Tilemap"};
        if (entity.HasComponent<LightComponent>())
            return {ICON_LC_LIGHTBULB, &Colors::NodeTint, "Light"};
        if (entity.HasComponent<MusicComponent>())
            return {ICON_LC_MUSIC, &Colors::NodeTint, "Music"};
        if (entity.HasComponent<AudioSourceComponent>())
            return {ICON_LC_VOLUME_2, &Colors::NodeTint, "Audio"};
        if (entity.HasComponent<ParticleEmitterComponent>())
            return {ICON_LC_SPARKLES, &Colors::NodeTint, "Particles"};
        if (entity.HasComponent<AnimatedSpriteComponent>())
            return {ICON_LC_FILM, &Colors::NodeTint, "Animated Sprite"};
        if (entity.HasComponent<SpriteComponent>())
            return {ICON_LC_IMAGE, &Colors::NodeTint, "Sprite"};
        if (entity.HasComponent<VectorShapeComponent>())
            return {ICON_LC_PEN_TOOL, &Colors::NodeTint, "Vector Shape"};
        if (entity.HasComponent<TextComponent>())
            return {ICON_LC_TYPE, &Colors::NodeTint, "Text"};
        if (entity.HasComponent<PhysicsComponent>())
            return {ICON_LC_ATOM, &Colors::NodeTint, "Body"};
        if (entity.HasComponent<RenderComponent>())
            return {ICON_LC_SQUARE, &Colors::NodeTint, "Shape"};
        if (entity.HasComponent<ShaderCanvasComponent>())
            return {ICON_LC_WAND_SPARKLES, &Colors::NodeTint, "Shader Canvas"};
        if (entity.HasComponent<CanvasComponent>())
            return {ICON_LC_MONITOR, &Colors::NodeTint, "Canvas"};
        if (entity.HasComponent<UIButtonComponent>())
            return {ICON_LC_SQUARE_MOUSE_POINTER, &Colors::NodeTint, "UI Button"};
        if (entity.HasComponent<UILabelComponent>())
            return {ICON_LC_TYPE, &Colors::NodeTint, "UI Label"};
        if (entity.HasComponent<UIImageComponent>())
            return {ICON_LC_IMAGE, &Colors::NodeTint, "UI Image"};
        if (entity.HasComponent<UILayoutComponent>())
            return {ICON_LC_LAYOUT_LIST, &Colors::NodeTint, "UI Layout"};
        if (entity.HasComponent<UIPanelComponent>())
            return {ICON_LC_PANEL_TOP, &Colors::NodeTint, "UI Panel"};
        if (entity.HasComponent<UIRectComponent>())
            return {ICON_LC_RECTANGLE_HORIZONTAL, &Colors::NodeTint, "UI Node"};
        if (entity.HasComponent<ButtonComponent>())
            return {ICON_LC_MOUSE_POINTER_CLICK, &Colors::NodeTint, "Button"};
        if (entity.HasComponent<TeleporterComponent>())
            return {ICON_LC_DOOR_OPEN, &Colors::NodeTint, "Teleporter"};
        if (entity.HasComponent<CameraZoneComponent>())
            return {ICON_LC_SCAN_EYE, &Colors::NodeTint, "Camera Zone"};
        if (entity.HasComponent<PickableComponent>())
            return {ICON_LC_GRAB, &Colors::NodeTint, "Pickable"};
        if (entity.HasComponent<CheckpointComponent>())
            return {ICON_LC_FLAG, &Colors::NodeTint, "Checkpoint"};
        if (entity.HasComponent<PickupComponent>())
            return {ICON_LC_GEM, &Colors::NodeTint, "Pickup"};
        if (entity.HasComponent<SpawnerComponent>())
            return {ICON_LC_COPY_PLUS, &Colors::NodeTint, "Spawner"};
        if (entity.HasComponent<DamageComponent>())
            return {ICON_LC_SWORDS, &Colors::NodeTint, "Damage"};
        if (entity.HasComponent<TriggerComponent>())
            return {ICON_LC_RADAR, &Colors::NodeTint, "Trigger"};
        if (entity.HasComponent<CollisionComponent>())
            return {ICON_LC_SCAN, &Colors::NodeTint, "Area"};
        return {ICON_LC_CIRCLE_DASHED, &Colors::TextDim, "Entity"};
    }
}
