#include "InspectorPanel.h"
#include <imgui.h>




InspectorPanel::InspectorPanel(
    EditorContext *context)
{
    m_Context = context;
}

void InspectorPanel::OnImGuiRender()
{
    auto &m_SelectedEntity =
        m_Context->SelectedEntity;

    ImGui::Begin("Inspector");

    if (m_SelectedEntity)
    {
        if (m_SelectedEntity.HasComponent<TagComponent>())
        {
            auto &tag = m_SelectedEntity.GetComponent<TagComponent>().Tag;

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, tag.c_str(), sizeof(buffer));

            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
                EditorCommands::RenameEntity(
                    *m_Context,
                    m_SelectedEntity,
                    buffer);
        }

        ImGui::Dummy(ImVec2(0,4));

        DrawComponentCard<TransformComponent>("Transform", m_SelectedEntity, [](auto &tc)
                                          {
    ImGui::DragFloat("X", &tc.position.x, 0.1f);
    ImGui::DragFloat("Y", &tc.position.y, 0.1f);
    ImGui::DragFloat("Rotation", &tc.rotation, 0.1f);
    ImGui::DragFloat("ScaleX", &tc.scale.x, 0.1f, 0.01f, 100.0f);
    ImGui::DragFloat("ScaleY", &tc.scale.y, 0.1f, 0.01f, 100.0f);
    ImGui::DragFloat("Z-Index", &tc.zIndex,1.0f, 0.0f, 10.0f); });

        DrawComponentCard<RenderComponent>("Render", m_SelectedEntity, [&](auto &rc)
                                       {
    float color[4] = {
        static_cast<float>(rc.color.r) / 255.0f,
        static_cast<float>(rc.color.g) / 255.0f,
        static_cast<float>(rc.color.b) / 255.0f,
        static_cast<float>(rc.color.a) / 255.0f
    };

    if (ImGui::ColorEdit4("Color", color))
    {
        rc.color.r = (Uint8)(color[0] * 255.0f);
        rc.color.g = (Uint8)(color[1] * 255.0f);
        rc.color.b = (Uint8)(color[2] * 255.0f);
        rc.color.a = (Uint8)(color[3] * 255.0f);
    }

    bool widthChanged = ImGui::DragFloat("Width", &rc.width, 0.1f);
    bool heightChanged = ImGui::DragFloat("Height", &rc.height, 0.1f);

    if (widthChanged || heightChanged)
    {
        // Use m_SelectedEntity directly (captured in lambda)
        if (m_SelectedEntity.HasComponent<SpriteComponent>())
        {
            auto &sprite = m_SelectedEntity.GetComponent<SpriteComponent>();
            sprite.width = rc.width;
            sprite.height = rc.height;
        }
    } });

        DrawComponentCard<VelocityComponent>("Velocity", m_SelectedEntity, [](auto &vc)
                                         { ImGui::DragFloat("Velocity X", &vc.vx, 0.1f); 
                                            ImGui::DragFloat("Velocity Y", &vc.vy, 0.1f); });

        DrawComponentCard<CameraComponent>("Camera", m_SelectedEntity, [](auto &cc)
                                       { ImGui::Checkbox("Primary", &cc.primary); });

        DrawComponentCard<CollisionComponent>("Collision", m_SelectedEntity, [](auto &col)
                                          {
                                              ImGui::DragFloat("Width", &col.width, 0.1f);
                                              ImGui::DragFloat("Height", &col.height, 0.1f);
                                              ImGui::DragFloat("Offset X", &col.offsetX, 0.1f);
                                              ImGui::DragFloat("Offset Y", &col.offsetY, 0.1f); });

        // sprite section
        DrawSpriteSection(m_SelectedEntity);

        DrawComponentCard<LifetimeComponent>("Lifetime",
                                         m_SelectedEntity,
                                         [](auto &lf)
                                         {
                                             ImGui::DragFloat(
                                                 "Remaining",
                                                 &lf.remaining,
                                                 0.1f,
                                                 0.0f,
                                                 10000.0f);
                                         });

        DrawComponentCard<AnimatedSpriteComponent>(
            "Animation",
            m_SelectedEntity,
            [](auto &ac)
            {
                if (!ac.animation)
                {
                    ImGui::Text("No Animation");
                    return;
                }

                bool loop = ac.animation->IsLooping();

                if (ImGui::Checkbox("Loop", &loop))
                {
                    ac.animation->SetLooping(loop);
                }

                float duration = ac.animation->GetFrameDuration();

                ImGui::DragFloat(
                    "Frame Duration",
                    &ac.animation->GetFrameDurationRef(),
                    0.01f);

                ImGui::DragFloat(
                    "Frame Duration",
                    &duration,
                    0.01f);
            });

        DrawComponentCard<PhysicsComponent>(
            "Physics",
            m_SelectedEntity,
            [](auto &pc)
            {
                if (!pc.body)
                {
                    ImGui::Text("No Body");
                    return;
                }

                auto *body = pc.body.get();

                ImGui::DragFloat2(
                    "Position",
                    &body->position.x);

                ImGui::DragFloat2(
                    "Velocity",
                    &body->velocity.x);

                ImGui::DragFloat(
                    "Rotation",
                    &body->rotation);

                ImGui::DragFloat(
                    "Inverse Mass",
                    &body->inverseMass);

                ImGui::DragFloat(
                    "Restitution",
                    &body->restitution);

                ImGui::DragFloat(
                    "Static Friction",
                    &body->staticFriction);

                ImGui::DragFloat(
                    "Dynamic Friction",
                    &body->dynamicFriction);

                ImGui::Checkbox(
                    "Lock Rotation",
                    &body->lockRotation);

                ImGui::Checkbox(
                    "Lock Position",
                    &body->lockPosition);
            });
    }

    ImGui::End();
}