#pragma once

#include "UI/Widgets.h"
#include "../EditorCommands.h"

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>

template <typename T, typename AddFn>
bool AddComponentMenuItem(EditorContext &ctx, SIMPEngine::Entity entity, const char *icon, const char *name, const char *filter, AddFn add)
{
    if (entity.HasComponent<T>() || !EditorUI::ContainsInsensitive(name, filter))
        return false;

    if (EditorUI::IconMenuItem(icon, EditorUI::Colors::NodeTint, name))
    {
        EditorCommands::RecordEdit(ctx);
        add();
        ctx.Scene->MarkPhysicsDirty();
    }
    return true;
}

inline void DrawAddComponentItems(EditorContext &ctx, SIMPEngine::Entity entity, const char *filter = nullptr)
{
    bool filtering = filter && *filter;
    bool any = false;

    if (!filtering)
        EditorUI::SectionLabel("Spatial");
    any |= AddComponentMenuItem<TransformComponent>(ctx, entity, ICON_LC_MOVE, "Transform", filter, [&]
                                                    { entity.AddComponent<TransformComponent>(); });
    any |= AddComponentMenuItem<VelocityComponent>(ctx, entity, ICON_LC_WIND, "Velocity", filter, [&]
                                                   { entity.AddComponent<VelocityComponent>(); });
    any |= AddComponentMenuItem<CameraComponent>(ctx, entity, ICON_LC_VIDEO, "Camera", filter, [&]
                                                 { entity.AddComponent<CameraComponent>(); });

    if (!filtering)
        EditorUI::SectionLabel("Rendering");
    any |= AddComponentMenuItem<RenderComponent>(ctx, entity, ICON_LC_SQUARE, "Shape", filter, [&]
                                                 { auto &r = entity.AddComponent<RenderComponent>(); r.width = 64.0f; r.height = 64.0f; });
    any |= AddComponentMenuItem<SpriteComponent>(ctx, entity, ICON_LC_IMAGE, "Sprite", filter, [&]
                                                 { auto &s = entity.AddComponent<SpriteComponent>(); s.width = 64.0f; s.height = 64.0f; });
    any |= AddComponentMenuItem<AnimatedSpriteComponent>(ctx, entity, ICON_LC_FILM, "Animated Sprite", filter, [&]
                                                         { entity.AddComponent<AnimatedSpriteComponent>(); });
    any |= AddComponentMenuItem<VectorShapeComponent>(ctx, entity, ICON_LC_PEN_TOOL, "Vector Shape", filter, [&]
                                                      { entity.AddComponent<VectorShapeComponent>(); });
    any |= AddComponentMenuItem<TextComponent>(ctx, entity, ICON_LC_TYPE, "Text", filter, [&]
                                               { entity.AddComponent<TextComponent>(); });
    any |= AddComponentMenuItem<TilemapComponent>(ctx, entity, ICON_LC_GRID_3X3, "Tilemap", filter, [&]
                                                  { entity.AddComponent<TilemapComponent>(); });
    any |= AddComponentMenuItem<ParticleEmitterComponent>(ctx, entity, ICON_LC_SPARKLES, "Particle Emitter", filter, [&]
                                                          { entity.AddComponent<ParticleEmitterComponent>(); });
    any |= AddComponentMenuItem<LightComponent>(ctx, entity, ICON_LC_LIGHTBULB, "Light", filter, [&]
                                                { entity.AddComponent<LightComponent>(); });
    any |= AddComponentMenuItem<GlowComponent>(ctx, entity, ICON_LC_SPARKLE, "Glow", filter, [&]
                                               { entity.AddComponent<GlowComponent>(); });
    any |= AddComponentMenuItem<ParallaxComponent>(ctx, entity, ICON_LC_LAYERS, "Parallax", filter, [&]
                                                   { entity.AddComponent<ParallaxComponent>(); });

    if (!filtering)
        EditorUI::SectionLabel("Audio");
    any |= AddComponentMenuItem<AudioSourceComponent>(ctx, entity, ICON_LC_VOLUME_2, "Audio Source", filter, [&]
                                                      { entity.AddComponent<AudioSourceComponent>(); });
    any |= AddComponentMenuItem<MusicComponent>(ctx, entity, ICON_LC_MUSIC, "Music", filter, [&]
                                                { entity.AddComponent<MusicComponent>(); });

    if (!filtering)
        EditorUI::SectionLabel("Simulation");
    any |= AddComponentMenuItem<CollisionComponent>(ctx, entity, ICON_LC_SCAN, "Collision", filter, [&]
                                                    { entity.AddComponent<CollisionComponent>(); });
    any |= AddComponentMenuItem<PhysicsComponent>(ctx, entity, ICON_LC_ATOM, "Rigid Body", filter, [&]
                                                  { entity.AddComponent<PhysicsComponent>(); });
    any |= AddComponentMenuItem<LifetimeComponent>(ctx, entity, ICON_LC_HOURGLASS, "Lifetime", filter, [&]
                                                   { entity.AddComponent<LifetimeComponent>(); });

    if (!filtering)
        EditorUI::SectionLabel("Logic");
    any |= AddComponentMenuItem<ScriptComponent>(ctx, entity, ICON_LC_CODE, "Script", filter, [&]
                                                 { entity.AddComponent<ScriptComponent>(); });
    any |= AddComponentMenuItem<TimerComponent>(ctx, entity, ICON_LC_TIMER, "Timer", filter, [&]
                                                { entity.AddComponent<TimerComponent>(); });
    any |= AddComponentMenuItem<TriggerComponent>(ctx, entity, ICON_LC_RADAR, "Trigger Zone", filter, [&]
                                                  { entity.AddComponent<TriggerComponent>(); });
    any |= AddComponentMenuItem<PathFollowerComponent>(ctx, entity, ICON_LC_ROUTE, "Path Follower", filter, [&]
                                                       { entity.AddComponent<PathFollowerComponent>(); });
    any |= AddComponentMenuItem<HealthComponent>(ctx, entity, ICON_LC_HEART, "Health", filter, [&]
                                                 { entity.AddComponent<HealthComponent>(); });
    any |= AddComponentMenuItem<ButtonComponent>(ctx, entity, ICON_LC_MOUSE_POINTER_CLICK, "Button", filter, [&]
                                                 { entity.AddComponent<ButtonComponent>(); });
    any |= AddComponentMenuItem<ShaderCanvasComponent>(ctx, entity, ICON_LC_WAND_SPARKLES, "Shader Canvas", filter, [&]
                                                       { entity.AddComponent<ShaderCanvasComponent>(); });
    any |= AddComponentMenuItem<CanvasComponent>(ctx, entity, ICON_LC_MONITOR, "Canvas", filter, [&]
                                                 { entity.AddComponent<CanvasComponent>(); });
    any |= AddComponentMenuItem<UIRectComponent>(ctx, entity, ICON_LC_RECTANGLE_HORIZONTAL, "UI Rect", filter, [&]
                                                 { entity.AddComponent<UIRectComponent>(); });
    any |= AddComponentMenuItem<UIPanelComponent>(ctx, entity, ICON_LC_PANEL_TOP, "UI Panel", filter, [&]
                                                  {
        entity.AddComponent<UIPanelComponent>();
        if (!entity.HasComponent<UIRectComponent>())
            entity.AddComponent<UIRectComponent>(); });
    any |= AddComponentMenuItem<UILabelComponent>(ctx, entity, ICON_LC_TYPE, "UI Label", filter, [&]
                                                  {
        entity.AddComponent<UILabelComponent>();
        if (!entity.HasComponent<UIRectComponent>())
            entity.AddComponent<UIRectComponent>(); });
    any |= AddComponentMenuItem<UIImageComponent>(ctx, entity, ICON_LC_IMAGE, "UI Image", filter, [&]
                                                  {
        entity.AddComponent<UIImageComponent>();
        if (!entity.HasComponent<UIRectComponent>())
            entity.AddComponent<UIRectComponent>(); });
    any |= AddComponentMenuItem<UIButtonComponent>(ctx, entity, ICON_LC_SQUARE_MOUSE_POINTER, "UI Button", filter, [&]
                                                   {
        entity.AddComponent<UIButtonComponent>();
        if (!entity.HasComponent<UIRectComponent>())
            entity.AddComponent<UIRectComponent>(); });
    any |= AddComponentMenuItem<UILayoutComponent>(ctx, entity, ICON_LC_LAYOUT_LIST, "UI Layout", filter, [&]
                                                   {
        entity.AddComponent<UILayoutComponent>();
        if (!entity.HasComponent<UIRectComponent>())
            entity.AddComponent<UIRectComponent>(); });
    any |= AddComponentMenuItem<TeleporterComponent>(ctx, entity, ICON_LC_DOOR_OPEN, "Teleporter", filter, [&]
                                                     {
        entity.AddComponent<TeleporterComponent>();
        if (!entity.HasComponent<TriggerComponent>())
        {
            auto &zone = entity.AddComponent<TriggerComponent>();
            zone.filter = "Player";
        } });
    any |= AddComponentMenuItem<CameraZoneComponent>(ctx, entity, ICON_LC_SCAN_EYE, "Camera Zone", filter, [&]
                                                     {
        entity.AddComponent<CameraZoneComponent>();
        if (!entity.HasComponent<TriggerComponent>())
        {
            auto &zone = entity.AddComponent<TriggerComponent>();
            zone.filter = "Player";
        } });
    any |= AddComponentMenuItem<WorldBoundsComponent>(ctx, entity, ICON_LC_SQUARE_DASHED, "World Bounds", filter, [&]
                                                      { entity.AddComponent<WorldBoundsComponent>(); });
    any |= AddComponentMenuItem<PickableComponent>(ctx, entity, ICON_LC_GRAB, "Pickable", filter, [&]
                                                   { entity.AddComponent<PickableComponent>(); });
    any |= AddComponentMenuItem<FollowComponent>(ctx, entity, ICON_LC_CROSSHAIR, "Follow", filter, [&]
                                                 { entity.AddComponent<FollowComponent>(); });
    any |= AddComponentMenuItem<PickupComponent>(ctx, entity, ICON_LC_GEM, "Pickup", filter, [&]
                                                 {
        entity.AddComponent<PickupComponent>();
        if (!entity.HasComponent<TriggerComponent>())
        {
            auto &zone = entity.AddComponent<TriggerComponent>();
            zone.filter = "Player";
        } });
    any |= AddComponentMenuItem<CheckpointComponent>(ctx, entity, ICON_LC_FLAG, "Checkpoint", filter, [&]
                                                     {
        entity.AddComponent<CheckpointComponent>();
        if (!entity.HasComponent<TriggerComponent>())
            entity.AddComponent<TriggerComponent>(); });
    any |= AddComponentMenuItem<RespawnComponent>(ctx, entity, ICON_LC_ROTATE_CCW, "Respawn", filter, [&]
                                                  { entity.AddComponent<RespawnComponent>(); });
    any |= AddComponentMenuItem<DamageComponent>(ctx, entity, ICON_LC_SWORDS, "Damage", filter, [&]
                                                 {
        entity.AddComponent<DamageComponent>();
        if (!entity.HasComponent<TriggerComponent>())
            entity.AddComponent<TriggerComponent>(); });
    any |= AddComponentMenuItem<OscillatorComponent>(ctx, entity, ICON_LC_ACTIVITY, "Oscillator", filter, [&]
                                                     { entity.AddComponent<OscillatorComponent>(); });
    any |= AddComponentMenuItem<SpawnerComponent>(ctx, entity, ICON_LC_COPY_PLUS, "Spawner", filter, [&]
                                                  { entity.AddComponent<SpawnerComponent>(); });
    any |= AddComponentMenuItem<CharacterControllerComponent>(ctx, entity, ICON_LC_GAMEPAD_2, "Character Controller", filter, [&]
                                                              {
        auto &controller = entity.AddComponent<CharacterControllerComponent>();
        if (entity.HasComponent<SpriteComponent>())
        {
            const auto &sprite = entity.GetComponent<SpriteComponent>();
            const glm::vec2 scale = glm::abs(entity.GetComponent<TransformComponent>().scale);
            if (sprite.width > 0.0f && sprite.height > 0.0f)
                controller.size = glm::vec2(sprite.width, sprite.height) * scale;
        } });

    if (!any)
        ImGui::TextDisabled(filtering ? "No match" : "Nothing to add");
}
