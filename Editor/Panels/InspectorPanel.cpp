#include "InspectorPanel.h"
#include <imgui.h>
#include "Core/Log.h"

template <typename T, typename UIFunc>
static void DrawComponent(const char *name, SIMPEngine::Entity entity, UIFunc uiFunc)
{
    if (entity.HasComponent<T>())
    {
        auto &component = entity.GetComponent<T>();

        if (ImGui::TreeNodeEx((void *)typeid(T).hash_code(),
                              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap,
                              name))
        {
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Remove Component"))
                    entity.RemoveComponent<T>();
                ImGui::EndPopup();
            }

            uiFunc(component);
            ImGui::TreePop();
        }
    }
}

void InspectorPanel::OnRender()
{
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
                tag = std::string(buffer);
        }

        ImGui::Separator();

        DrawComponent<TransformComponent>("Transform", m_SelectedEntity, [](auto &tc)
                                          {
    ImGui::DragFloat("X", &tc.position.x, 0.1f);
    ImGui::DragFloat("Y", &tc.position.y, 0.1f);
    ImGui::DragFloat("Rotation", &tc.rotation, 0.1f);
    ImGui::DragFloat("ScaleX", &tc.scale.x, 0.1f, 0.01f, 100.0f);
    ImGui::DragFloat("ScaleY", &tc.scale.y, 0.1f, 0.01f, 100.0f); });

        DrawComponent<RenderComponent>("Render", m_SelectedEntity, [&](auto &rc)
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

        DrawComponent<VelocityComponent>("Velocity", m_SelectedEntity, [](auto &vc)
                                         { ImGui::DragFloat("Velocity X", &vc.vx, 0.1f); 
                                            ImGui::DragFloat("Velocity Y", &vc.vy, 0.1f); });

        DrawComponent<CameraComponent>("Camera", m_SelectedEntity, [](auto &cc)
                                       { ImGui::Checkbox("Primary", &cc.primary); });

        DrawComponent<CollisionComponent>("Collision", m_SelectedEntity, [](auto &col)
                                          {
            ImGui::DragFloat2("Size", &col.width, 0.1f);
            ImGui::DragFloat2("Offset", &col.width, 0.1f); });
    }

    ImGui::End();
}
