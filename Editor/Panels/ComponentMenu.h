#pragma once

#include "UI/Widgets.h"
#include "../EditorCommands.h"

#include <Engine/Scene/Component.h>
#include <Engine/Scene/Entity.h>

template <typename T, typename AddFn>
bool AddComponentMenuItem(EditorContext &ctx, SIMPEngine::Entity entity, const char *name, const char *filter, AddFn add)
{
    if (entity.HasComponent<T>() || !EditorUI::ContainsInsensitive(name, filter))
        return false;

    if (ImGui::MenuItem(name))
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
    any |= AddComponentMenuItem<TransformComponent>(ctx, entity, "Transform", filter, [&]
                                                    { entity.AddComponent<TransformComponent>(); });
    any |= AddComponentMenuItem<VelocityComponent>(ctx, entity, "Velocity", filter, [&]
                                                   { entity.AddComponent<VelocityComponent>(); });
    any |= AddComponentMenuItem<CameraComponent>(ctx, entity, "Camera", filter, [&]
                                                 { entity.AddComponent<CameraComponent>(); });

    if (!filtering)
        EditorUI::SectionLabel("Rendering");
    any |= AddComponentMenuItem<RenderComponent>(ctx, entity, "Shape", filter, [&]
                                                 { auto &r = entity.AddComponent<RenderComponent>(); r.width = 64.0f; r.height = 64.0f; });
    any |= AddComponentMenuItem<SpriteComponent>(ctx, entity, "Sprite", filter, [&]
                                                 { auto &s = entity.AddComponent<SpriteComponent>(); s.width = 64.0f; s.height = 64.0f; });
    any |= AddComponentMenuItem<AnimatedSpriteComponent>(ctx, entity, "Animated Sprite", filter, [&]
                                                         { entity.AddComponent<AnimatedSpriteComponent>(); });
    any |= AddComponentMenuItem<TextComponent>(ctx, entity, "Text", filter, [&]
                                               { entity.AddComponent<TextComponent>(); });
    any |= AddComponentMenuItem<TilemapComponent>(ctx, entity, "Tilemap", filter, [&]
                                                  { entity.AddComponent<TilemapComponent>(); });

    if (!filtering)
        EditorUI::SectionLabel("Simulation");
    any |= AddComponentMenuItem<CollisionComponent>(ctx, entity, "Collision", filter, [&]
                                                    { entity.AddComponent<CollisionComponent>(); });
    any |= AddComponentMenuItem<PhysicsComponent>(ctx, entity, "Rigid Body", filter, [&]
                                                  { entity.AddComponent<PhysicsComponent>(); });
    any |= AddComponentMenuItem<LifetimeComponent>(ctx, entity, "Lifetime", filter, [&]
                                                   { entity.AddComponent<LifetimeComponent>(); });

    if (!any)
        ImGui::TextDisabled(filtering ? "No match" : "Nothing to add");
}
