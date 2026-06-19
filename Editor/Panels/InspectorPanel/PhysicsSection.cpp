#include "InspectorPanel.h"
#include <AccelEngine/world.h>
#include <imgui.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const char* ShapeLabel(AccelEngine::ShapeType t)
{
    switch (t)
    {
        case AccelEngine::ShapeType::CIRCLE: return "Circle";
        case AccelEngine::ShapeType::AABB:   return "AABB";
        default:                             return "Unknown";
    }
}

static void DrawBodyCreator(SIMPEngine::Entity& entity)
{
    ImGui::TextDisabled("No physics body attached.");
    ImGui::Spacing();

    ImGui::TextUnformatted("Add Body");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Add AABB Body", ImVec2(-1, 28)))
    {
        if (!entity.HasComponent<PhysicsComponent>())
            entity.AddComponent<PhysicsComponent>();

        auto& phys  = entity.GetComponent<PhysicsComponent>();
        phys.body   = std::make_unique<AccelEngine::RigidBody>();

        phys.body->entityID   = (uint32_t)entity.GetHandle();
        phys.body->shapeType  = AccelEngine::ShapeType::AABB;
        phys.body->aabb.halfSize = { 0.5f, 0.5f };
        phys.body->inverseMass   = 1.0f;
        phys.body->restitution   = 0.3f;
        phys.body->staticFriction  = 0.5f;
        phys.body->dynamicFriction = 0.3f;
        phys.body->calculateInertia();
    }

    ImGui::Spacing();

    if (ImGui::Button("Add Circle Body", ImVec2(-1, 28)))
    {
        if (!entity.HasComponent<PhysicsComponent>())
            entity.AddComponent<PhysicsComponent>();

        auto& phys  = entity.GetComponent<PhysicsComponent>();
        phys.body   = std::make_unique<AccelEngine::RigidBody>();

        phys.body->entityID   = (uint32_t)entity.GetHandle();
        phys.body->shapeType  = AccelEngine::ShapeType::CIRCLE;
        phys.body->circle.radius = 0.5f;
        phys.body->inverseMass   = 1.0f;
        phys.body->restitution   = 0.3f;
        phys.body->staticFriction  = 0.5f;
        phys.body->dynamicFriction = 0.3f;
        phys.body->calculateInertia();
    }

    ImGui::Spacing();

    // Static (infinite mass) convenience shortcut
    if (ImGui::Button("Add Static AABB Body", ImVec2(-1, 28)))
    {
        if (!entity.HasComponent<PhysicsComponent>())
            entity.AddComponent<PhysicsComponent>();

        auto& phys  = entity.GetComponent<PhysicsComponent>();
        phys.body   = std::make_unique<AccelEngine::RigidBody>();

        phys.body->entityID   = (uint32_t)entity.GetHandle();
        phys.body->shapeType  = AccelEngine::ShapeType::AABB;
        phys.body->aabb.halfSize = { 0.5f, 0.5f };
        phys.body->inverseMass   = 0.0f;   // static
        phys.body->restitution   = 0.3f;
        phys.body->staticFriction  = 0.8f;
        phys.body->dynamicFriction = 0.6f;
        phys.body->lockRotation  = true;
        phys.body->lockPosition  = true;
        phys.body->calculateInertia();
    }
}

static void DrawBodyControls(AccelEngine::RigidBody* body)
{
    // ------------------------------------------------------------------
    // Shape info (read-only label + shape-specific size)
    // ------------------------------------------------------------------
    ImGui::TextUnformatted("Shape");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextDisabled("Type: %s", ShapeLabel(body->shapeType));
    ImGui::Spacing();

    if (body->shapeType == AccelEngine::ShapeType::AABB)
    {
        if (ImGui::DragFloat2("Half Size", &body->aabb.halfSize.x, 0.01f, 0.001f, 1000.0f))
            body->calculateInertia();
    }
    else if (body->shapeType == AccelEngine::ShapeType::CIRCLE)
    {
        if (ImGui::DragFloat("Radius", &body->circle.radius, 0.01f, 0.001f, 1000.0f))
            body->calculateInertia();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // ------------------------------------------------------------------
    // Transform
    // ------------------------------------------------------------------
    ImGui::TextUnformatted("Transform");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::DragFloat2("Position",    &body->position.x,    0.01f);
    ImGui::DragFloat ("Rotation",    &body->rotation,       0.1f);

    ImGui::Spacing();
    ImGui::Spacing();

    // ------------------------------------------------------------------
    // Motion
    // ------------------------------------------------------------------
    ImGui::TextUnformatted("Motion");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::DragFloat2("Velocity",    &body->velocity.x,    0.01f);

    // Display mass as a friendlier value; 0 inverse mass = static
    float mass = (body->inverseMass > 0.0f) ? (1.0f / body->inverseMass) : 0.0f;
    if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.0f, 100000.0f, "%.3f"))
    {
        body->inverseMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
        body->calculateInertia();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(0 = static)");

    ImGui::Spacing();
    ImGui::Spacing();

    // ------------------------------------------------------------------
    // Material
    // ------------------------------------------------------------------
    ImGui::TextUnformatted("Material");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::DragFloat("Restitution",      &body->restitution,      0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Static Friction",  &body->staticFriction,   0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Dynamic Friction", &body->dynamicFriction,  0.01f, 0.0f, 1.0f);

    ImGui::Spacing();
    ImGui::Spacing();

    // ------------------------------------------------------------------
    // Constraints
    // ------------------------------------------------------------------
    ImGui::TextUnformatted("Constraints");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Lock Rotation", &body->lockRotation);
    ImGui::SameLine(0, 20);
    ImGui::Checkbox("Lock Position", &body->lockPosition);

    ImGui::Spacing();
    ImGui::Spacing();

    // ------------------------------------------------------------------
    // Runtime controls
    // ------------------------------------------------------------------
    ImGui::TextUnformatted("Runtime");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Zero Velocity", ImVec2(-1, 26)))
    {
        body->velocity = { 0.0f, 0.0f };
    }

    if (ImGui::Button("Reset Rotation", ImVec2(-1, 26)))
    {
        body->rotation = 0.0f;
    }
}

// ---------------------------------------------------------------------------
// Public entry point called from InspectorPanel::OnImGuiRender
// ---------------------------------------------------------------------------

void InspectorPanel::DrawPhysicsSection(SIMPEngine::Entity& m_SelectedEntity)
{
    // If the entity has no PhysicsComponent at all, show a lightweight
    // "Add Component" section outside of DrawComponentCard so it always
    // appears even when the component is absent.
    if (!m_SelectedEntity.HasComponent<PhysicsComponent>())
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));

        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.22f, 0.27f, 0.36f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.28f, 0.34f, 0.46f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.35f, 0.45f, 0.65f, 1.0f));

        bool open = ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        if (open)
        {
            ImGui::PushID("PhysicsAdd");
            ImGui::Dummy(ImVec2(0, 2));
            DrawBodyCreator(m_SelectedEntity);
            ImGui::Dummy(ImVec2(0, 2));
            ImGui::PopID();
        }

        return;
    }

    // Component exists — use the standard card.
    DrawComponentCard<PhysicsComponent>(
        "Physics",
        m_SelectedEntity,
        [&m_SelectedEntity](PhysicsComponent& pc)
        {
            if (!pc.body)
            {
                // Component exists but body was never constructed (edge case).
                DrawBodyCreator(m_SelectedEntity);
                return;
            }

            DrawBodyControls(pc.body.get());

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Destroy body (keeps component, just nulls the body)
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.18f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.22f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.42f, 0.14f, 0.14f, 1.0f));

            if (ImGui::Button("Remove Body", ImVec2(-1, 28)))
                pc.body.reset();

            ImGui::PopStyleColor(3);
        });
}