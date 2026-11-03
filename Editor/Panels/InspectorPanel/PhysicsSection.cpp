#include "InspectorPanel.h"
#include <AccelEngine/world.h>
#include <Engine/Math/EntityUtility.h>
#include <imgui.h>

#include <cmath>

enum class BodyPreset { DynamicBox, DynamicCircle, StaticBox };

static void CreateBody(EditorContext &ctx, SIMPEngine::Entity &entity, BodyPreset preset)
{
    EditorCommands::RecordEdit(ctx);

    if (!entity.HasComponent<PhysicsComponent>())
        entity.AddComponent<PhysicsComponent>();

    auto &phys = entity.GetComponent<PhysicsComponent>();
    phys.body = std::make_unique<AccelEngine::RigidBody>();

    glm::vec2 half(0.5f);
    if (auto size = SIMPEngine::GetEntityLocalSize(entity))
        half = glm::max(*size * 0.5f * glm::abs(entity.GetComponent<TransformComponent>().scale), glm::vec2(0.5f));

    phys.body->entityID = static_cast<uint32_t>(entity.GetHandle());
    phys.body->restitution = 0.3f;
    phys.body->staticFriction = 0.5f;
    phys.body->dynamicFriction = 0.3f;

    switch (preset)
    {
    case BodyPreset::DynamicBox:
        phys.body->shapeType = AccelEngine::ShapeType::AABB;
        phys.body->aabb.halfSize = {half.x, half.y};
        phys.body->inverseMass = 1.0f;
        break;
    case BodyPreset::DynamicCircle:
        phys.body->shapeType = AccelEngine::ShapeType::CIRCLE;
        phys.body->circle.radius = std::max(half.x, half.y);
        phys.body->inverseMass = 1.0f;
        break;
    case BodyPreset::StaticBox:
        phys.body->shapeType = AccelEngine::ShapeType::AABB;
        phys.body->aabb.halfSize = {half.x, half.y};
        phys.body->inverseMass = 0.0f;
        phys.body->staticFriction = 0.8f;
        phys.body->dynamicFriction = 0.6f;
        phys.body->lockRotation = true;
        phys.body->lockPosition = true;
        break;
    }

    phys.body->calculateInertia();
    ctx.Scene->SyncPhysicsFromTransforms();
    ctx.Scene->MarkPhysicsDirty();
}

static void DrawBodyCreator(EditorContext &ctx, SIMPEngine::Entity &entity)
{
    using namespace EditorUI;

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDim);
    ImGui::TextWrapped("No body yet. Pick a starting shape:");
    ImGui::PopStyleColor();

    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float w = std::floor((ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f);
    if (Button("Box", ButtonKind::Secondary, ImVec2(w, 0)))
        CreateBody(ctx, entity, BodyPreset::DynamicBox);
    ImGui::SameLine();
    if (Button("Circle", ButtonKind::Secondary, ImVec2(w, 0)))
        CreateBody(ctx, entity, BodyPreset::DynamicCircle);
    ImGui::SameLine();
    if (Button("Static", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
        CreateBody(ctx, entity, BodyPreset::StaticBox);
}

static void DrawBodyControls(EditorContext &ctx, AccelEngine::RigidBody *body)
{
    using namespace EditorUI;

    static const char *shapes[] = {"Box", "Circle"};
    int shape = body->shapeType == AccelEngine::ShapeType::CIRCLE ? 1 : 0;
    if (PropertySegmented("Shape", &shape, shapes, 2))
    {
        body->shapeType = shape == 1 ? AccelEngine::ShapeType::CIRCLE : AccelEngine::ShapeType::AABB;
        if (shape == 1 && body->circle.radius <= 0.0f)
            body->circle.radius = std::max(body->aabb.halfSize.x, body->aabb.halfSize.y);
        if (shape == 0 && body->aabb.halfSize.x <= 0.0f)
            body->aabb.halfSize = {body->circle.radius, body->circle.radius};
        body->calculateInertia();
    }

    if (body->shapeType == AccelEngine::ShapeType::AABB)
    {
        if (PropertyVec2("Half Size", &body->aabb.halfSize.x, 0.5f, "%.1f", 0.01f, 100000.0f))
            body->calculateInertia();
    }
    else
    {
        if (PropertyFloat("Radius", &body->circle.radius, 0.5f, 0.01f, 100000.0f, "%.1f"))
            body->calculateInertia();
    }

    bool isStatic = body->inverseMass <= 0.0f;
    float mass = isStatic ? 0.0f : 1.0f / body->inverseMass;
    if (PropertyFloat("Mass", &mass, 0.1f, 0.0f, 100000.0f, mass > 0.0f ? "%.2f" : "static"))
    {
        body->inverseMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
        body->calculateInertia();
    }

    PropertyVec2("Velocity", &body->velocity.x, 0.5f, "%.1f");
    PropertyFloat("Spin", &body->rotation, 0.05f, 0.0f, 0.0f, "%.2f rad/s");

    PropertyFloat("Bounce", &body->restitution, 0.01f, 0.0f, 1.0f);
    PropertyFloat("Friction (static)", &body->staticFriction, 0.01f, 0.0f, 1.0f);
    PropertyFloat("Friction (moving)", &body->dynamicFriction, 0.01f, 0.0f, 1.0f);
    PropertyBool2("Lock", "Rotation", &body->lockRotation, "Position", &body->lockPosition);

    PropertyLabel("");
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float half = std::floor((ImGui::GetContentRegionAvail().x - spacing) * 0.5f);
    if (Button("Stop motion", ButtonKind::Secondary, ImVec2(half, 0)))
    {
        EditorCommands::RecordEdit(ctx);
        body->velocity = {0.0f, 0.0f};
        body->rotation = 0.0f;
    }
    ImGui::SameLine();
    if (Button("Reset angle", ButtonKind::Secondary, ImVec2(-FLT_MIN, 0)))
    {
        EditorCommands::RecordEdit(ctx);
        body->orientation = 0.0f;
        body->calculateDerivativeData();
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

            DrawBodyControls(ctx, pc.body.get());

            PropertyLabel("");
            if (Button("Remove body", ButtonKind::Danger, ImVec2(-FLT_MIN, 0)))
            {
                EditorCommands::RecordEdit(ctx);
                pc.body.reset();
                ctx.Scene->MarkPhysicsDirty();
            }
        });
}
