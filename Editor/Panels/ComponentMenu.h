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

    if (!any)
        ImGui::TextDisabled(filtering ? "No match" : "Nothing to add");
}
