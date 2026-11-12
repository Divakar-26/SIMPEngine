#include "InspectorPanel.h"
#include <AccelEngine/world.h>
#include <Engine/Math/EntityUtility.h>
#include <Engine/Physics/BodyFactory.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
    using AccelEngine::RigidBody;
    using AccelEngine::ShapeType;

    enum class BodyPreset { Box, Circle, Capsule, Polygon, Static, Platform };

    SIMPEngine::PhysicsBodies::Options OptionsFrom(const RigidBody &body)
    {
        SIMPEngine::PhysicsBodies::Options options;
        options.Mass = body.getMass();
        options.Restitution = body.restitution;
        options.StaticFriction = body.staticFriction;
        options.DynamicFriction = body.dynamicFriction;
        return options;
    }

    void CopyBehaviour(const RigidBody &from, RigidBody &to)
    {
        to.position = from.position;
        to.orientation = from.orientation;
        to.velocity = from.velocity;
        to.rotation = from.rotation;
        to.linearDamping = from.linearDamping;
        to.angularDamping = from.angularDamping;
        to.lockPosition = from.lockPosition;
        to.lockRotation = from.lockRotation;
        to.ignoreGravity = from.ignoreGravity;
        to.canSleep = from.canSleep;
        to.categoryBits = from.categoryBits;
        to.maskBits = from.maskBits;
        to.groupIndex = from.groupIndex;
        to.isSensor = from.isSensor;
        to.oneWay = from.oneWay;
        to.oneWayNormal = from.oneWayNormal;
        to.enableCollision = from.enableCollision;
        to.calculateDerivativeData();
    }

    void ReplaceShape(EditorContext &ctx, SIMPEngine::Entity &entity, std::unique_ptr<RigidBody> replacement)
    {
        if (!replacement)
            return;
        auto &phys = entity.GetComponent<PhysicsComponent>();
        if (phys.body)
            CopyBehaviour(*phys.body, *replacement);
        ctx.Scene->AddBody(entity.GetHandle(), std::move(replacement));
    }

    void CreateBody(EditorContext &ctx, SIMPEngine::Entity &entity, BodyPreset preset)
    {
        EditorCommands::RecordEdit(ctx);

        if (!entity.HasComponent<PhysicsComponent>())
            entity.AddComponent<PhysicsComponent>();

        glm::vec2 half(0.5f);
        if (auto size = SIMPEngine::GetEntityLocalSize(entity))
            half = glm::max(*size * 0.5f * glm::abs(entity.GetComponent<TransformComponent>().scale), glm::vec2(0.5f));

        SIMPEngine::PhysicsBodies::Options options;
        std::unique_ptr<RigidBody> body;
        switch (preset)
        {
        case BodyPreset::Box:
            body = SIMPEngine::PhysicsBodies::Box(half, options);
            break;
        case BodyPreset::Circle:
            body = SIMPEngine::PhysicsBodies::Circle(std::max(half.x, half.y), options);
            break;
        case BodyPreset::Capsule:
        {
            const float radius = std::min(half.x, half.y);
            const float halfLength = std::max(std::max(half.x, half.y) - radius, 0.0f);
            body = SIMPEngine::PhysicsBodies::Capsule(halfLength, radius, options);
            break;
        }
        case BodyPreset::Polygon:
            body = SIMPEngine::PhysicsBodies::RegularPolygon(6, std::max(half.x, half.y), options);
            break;
        case BodyPreset::Static:
        case BodyPreset::Platform:
            options.Mass = 0.0f;
            options.StaticFriction = 0.8f;
            options.DynamicFriction = 0.6f;
            body = SIMPEngine::PhysicsBodies::Box(half, options);
            body->lockRotation = true;
            body->lockPosition = true;
            if (preset == BodyPreset::Platform)
                body->oneWay = true;
            break;
        }

        ctx.Scene->AddBody(entity.GetHandle(), std::move(body));
        ctx.Scene->SyncPhysicsFromTransforms();
    }

    void DrawBodyCreator(EditorContext &ctx, SIMPEngine::Entity &entity)
    {
        using namespace EditorUI;

        ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
        ImGui::TextWrapped("No body yet. Pick a starting shape:");
        ImGui::PopStyleColor();

        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float w = std::floor((ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f);
        if (Button("Box", ButtonKind::Secondary, ImVec2(w, 0)))
            CreateBody(ctx, entity, BodyPreset::Box);
        ImGui::SameLine();
        if (Button("Circle", ButtonKind::Secondary, ImVec2(w, 0)))
            CreateBody(ctx, entity, BodyPreset::Circle);
        ImGui::SameLine();
        if (Button("Capsule", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
            CreateBody(ctx, entity, BodyPreset::Capsule);
        if (Button("Polygon", ButtonKind::Secondary, ImVec2(w, 0)))
            CreateBody(ctx, entity, BodyPreset::Polygon);
        ImGui::SameLine();
        if (Button("Static", ButtonKind::Secondary, ImVec2(w, 0)))
            CreateBody(ctx, entity, BodyPreset::Static);
        ImGui::SameLine();
        if (Button("One-Way", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
            CreateBody(ctx, entity, BodyPreset::Platform);
    }

    void DrawShapeControls(EditorContext &ctx, SIMPEngine::Entity &entity, RigidBody *body)
    {
        using namespace EditorUI;

        static const char *shapes[] = {"Box", "Circle", "Capsule", "Polygon"};
        const bool compound = body->shapeType == ShapeType::COMPOUND;
        int shape = body->shapeType == ShapeType::AABB ? 0 : body->shapeType == ShapeType::CIRCLE ? 1 : body->shapeType == ShapeType::CAPSULE ? 2 : 3;

        if (compound)
        {
            char info[64];
            std::snprintf(info, sizeof(info), "Compound, %zu parts", body->parts.size());
            PropertyValue("Shape", info);
            return;
        }

        const glm::vec2 extents = SIMPEngine::PhysicsBodies::ShapeExtents(*body);
        const glm::vec2 half = glm::max(extents * 0.5f, glm::vec2(0.5f));
        const SIMPEngine::PhysicsBodies::Options options = OptionsFrom(*body);

        if (PropertySegmented("Shape", &shape, shapes, 4))
        {
            EditorCommands::RecordEdit(ctx);
            std::unique_ptr<RigidBody> replacement;
            if (shape == 0)
                replacement = SIMPEngine::PhysicsBodies::Box(half, options);
            else if (shape == 1)
                replacement = SIMPEngine::PhysicsBodies::Circle(std::max(half.x, half.y), options);
            else if (shape == 2)
            {
                const float radius = std::min(half.x, half.y);
                replacement = SIMPEngine::PhysicsBodies::Capsule(std::max(std::max(half.x, half.y) - radius, 0.0f), radius, options);
            }
            else
                replacement = SIMPEngine::PhysicsBodies::RegularPolygon(6, std::max(half.x, half.y), options);
            ReplaceShape(ctx, entity, std::move(replacement));
            return;
        }

        if (body->shapeType == ShapeType::AABB)
        {
            if (PropertyVec2("Half Size", &body->aabb.halfSize.x, 0.5f, "%.1f", 0.01f, 100000.0f))
                body->calculateInertia();
        }
        else if (body->shapeType == ShapeType::CIRCLE)
        {
            if (PropertyFloat("Radius", &body->circle.radius, 0.5f, 0.01f, 100000.0f, "%.1f"))
                body->calculateInertia();
        }
        else if (body->shapeType == ShapeType::CAPSULE && !body->parts.empty())
        {
            const auto &part = body->parts.front();
            float halfLength = part.count >= 2 ? (part.v[1] - part.v[0]).magnitude() * 0.5f : 0.0f;
            float radius = part.radius;
            bool changed = PropertyFloat("Half Length", &halfLength, 0.5f, 0.0f, 100000.0f, "%.1f");
            changed |= PropertyFloat("Radius", &radius, 0.5f, 0.01f, 100000.0f, "%.1f");
            if (changed)
            {
                EditorCommands::RecordEdit(ctx);
                ReplaceShape(ctx, entity, SIMPEngine::PhysicsBodies::Capsule(halfLength, radius, options));
                return;
            }
        }
        else if (!body->parts.empty())
        {
            int sides = body->parts.front().count;
            float radius = body->parts.front().boundRadius;
            bool changed = PropertyInt("Sides", &sides, 0.1f, 3, 8);
            changed |= PropertyFloat("Radius", &radius, 0.5f, 0.5f, 100000.0f, "%.1f");
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Editing these rebuilds the shape as a regular polygon");
            if (changed)
            {
                EditorCommands::RecordEdit(ctx);
                ReplaceShape(ctx, entity, SIMPEngine::PhysicsBodies::RegularPolygon(sides, radius, options));
                return;
            }
        }
    }

    void DrawLayerMask(const char *label, uint16_t *mask)
    {
        using namespace EditorUI;
        PropertyLabel(label);
        ImGui::PushID(label);
        for (int row = 0; row < 2; ++row)
        {
            for (int column = 0; column < 8; ++column)
            {
                const int bit = row * 8 + column;
                bool on = (*mask & (1u << bit)) != 0;
                ImGui::PushID(bit);
                if (ImGui::Checkbox("##bit", &on))
                {
                    if (on)
                        *mask = static_cast<uint16_t>(*mask | (1u << bit));
                    else
                        *mask = static_cast<uint16_t>(*mask & ~(1u << bit));
                    NotifyEdit();
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                    ImGui::SetTooltip("Layer %d", bit);
                ImGui::PopID();
                if (column < 7)
                    ImGui::SameLine(0.0f, 2.0f);
            }
        }
        ImGui::PopID();
    }

    void DrawBodyControls(EditorContext &ctx, SIMPEngine::Entity &entity, RigidBody *body)
    {
        using namespace EditorUI;

        DrawShapeControls(ctx, entity, body);
        body = entity.GetComponent<PhysicsComponent>().body.get();
        if (!body)
            return;

        const bool isStatic = body->inverseMass <= 0.0f;
        float mass = isStatic ? 0.0f : 1.0f / body->inverseMass;
        if (PropertyFloat("Mass", &mass, 0.1f, 0.0f, 100000.0f, mass > 0.0f ? "%.2f" : "static"))
            body->setMass(mass);

        SectionLabel("Material");
        PropertyFloat("Bounce", &body->restitution, 0.01f, 0.0f, 1.0f);
        PropertyFloat("Friction (static)", &body->staticFriction, 0.01f, 0.0f, 2.0f);
        PropertyFloat("Friction (moving)", &body->dynamicFriction, 0.01f, 0.0f, 2.0f);
        PropertyFloat("Linear Damping", &body->linearDamping, 0.005f, 0.0f, 1.0f, "%.3f");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Fraction of the speed kept per second. 1 = no air drag, 0.5 = loses half every second");
        PropertyFloat("Angular Damping", &body->angularDamping, 0.005f, 0.0f, 1.0f, "%.3f");

        SectionLabel("Behaviour");
        PropertyBool2("Lock", "Rotation", &body->lockRotation, "Position", &body->lockPosition);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Locking both makes it a static body that still follows its Transform (moving platforms)");
        PropertyBool("Ignore Gravity", &body->ignoreGravity);
        PropertyBool("Can Sleep", &body->canSleep);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Resting bodies stop being simulated until something touches them. Turn off for bodies a script drives every frame");

        SectionLabel("Collision");
        PropertyBool("Sensor", &body->isSensor);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Detects overlaps without pushing anything. Scripts get OnTriggerEnter, OnTriggerStay and OnTriggerExit");
        PropertyBool("One-Way Platform", &body->oneWay);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Solid only for bodies coming from the side the arrow points to (up by default). Use thin shapes");
        if (body->oneWay)
        {
            if (PropertyVec2("Pass Direction", &body->oneWayNormal.x, 0.05f, "%.2f", -1.0f, 1.0f))
                if (body->oneWayNormal.squareMagnitude() < 1e-6f)
                    body->oneWayNormal = {0.0f, 1.0f};
        }

        int category = 0;
        while (category < 15 && !(body->categoryBits & (1u << category)))
            ++category;
        if (PropertyInt("Layer", &category, 0.1f, 0, 15))
            body->categoryBits = static_cast<uint16_t>(1u << std::clamp(category, 0, 15));
        DrawLayerMask("Collides With", &body->maskBits);
        int group = body->groupIndex;
        if (PropertyInt("Group", &group, 0.1f, -100, 100))
            body->groupIndex = static_cast<int16_t>(group);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            ImGui::SetTooltip("Bodies in the same positive group always collide, in the same negative group never");

        SectionLabel("Motion");
        PropertyVec2("Velocity", &body->velocity.x, 0.5f, "%.1f");
        PropertyFloat("Spin", &body->rotation, 0.05f, -300.0f, 300.0f, "%.2f rad/s");

        if (!ctx.IsEditing())
            PropertyValue("State", body->isAwake ? "Awake" : "Asleep", body->isAwake ? &Colors::Success : &Colors::TextDim);

        PropertyLabel("");
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float half = std::floor((ImGui::GetContentRegionAvail().x - spacing) * 0.5f);
        if (Button("Stop motion", ButtonKind::Secondary, ImVec2(half, 0)))
        {
            EditorCommands::RecordEdit(ctx);
            body->setVelocity({0.0f, 0.0f}, 0.0f);
        }
        ImGui::SameLine();
        if (Button("Reset angle", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        {
            EditorCommands::RecordEdit(ctx);
            entity.GetComponent<TransformComponent>().rotation = 0.0f;
            entity.GetComponent<TransformComponent>().dirty = true;
            body->orientation = 0.0f;
            body->calculateDerivativeData();
        }
    }
}

void InspectorPanel::DrawPhysicsSection(SIMPEngine::Entity &m_SelectedEntity)
{
    using namespace EditorUI;
    EditorContext &ctx = *m_Context;

    DrawComponentSection<PhysicsComponent>(
        ctx,
        "Rigid Body",
        m_SelectedEntity,
        [&ctx, &m_SelectedEntity](PhysicsComponent &pc)
        {
            if (!pc.body)
            {
                DrawBodyCreator(ctx, m_SelectedEntity);
                return;
            }

            DrawBodyControls(ctx, m_SelectedEntity, pc.body.get());

            PropertyLabel("");
            if (Button("Remove body", ButtonKind::Danger, ImVec2(-FLT_MIN, 0)))
            {
                EditorCommands::RecordEdit(ctx);
                ctx.Scene->RemoveBody(m_SelectedEntity.GetHandle());
            }
        });
}
