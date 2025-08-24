#include "InspectorPanel.h"
#include <imgui.h>
#include "Log.h"

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
    ImGui::DragFloat("X", &tc.x, 0.1f);
    ImGui::DragFloat("Y", &tc.y, 0.1f);
    ImGui::DragFloat("Rotation", &tc.rotation, 0.1f);
    ImGui::DragFloat("ScaleX", &tc.scaleX, 0.1f, 0.01f, 100.0f);
    ImGui::DragFloat("ScaleY", &tc.scaleY, 0.1f, 0.01f, 100.0f); });

        DrawComponent<RenderComponent>("Render", m_SelectedEntity, [](auto &sc)
                                               {
        float color[4] = {
            static_cast<float>(sc.color.r) / 255.0f,
            static_cast<float>(sc.color.g) / 255.0f,
            static_cast<float>(sc.color.b) / 255.0f,
            static_cast<float>(sc.color.a) / 255.0f
        };

            CORE_INFO("Float alpha: {}", color[3]);


        if (ImGui::ColorEdit4("Color", color))
        {
            sc.color.r = (Uint8)(color[0] * 255.0f);
            sc.color.g = (Uint8)(color[1] * 255.0f);
            sc.color.b = (Uint8)(color[2] * 255.0f);
            sc.color.a = (Uint8)(color[3] * 255.0f);
        
            CORE_INFO("Updated SDL_Color: r={} g={} b={} a={}", 
                      (int)sc.color.r, (int)sc.color.g, (int)sc.color.b, (int)sc.color.a);
        }


    // sc.color = SDL_Color{255,0,0,255};

    ImGui::DragFloat("Width", &sc.width, 0.1f);
    ImGui::DragFloat("Height", &sc.height, 0.1f); });

        DrawComponent<VelocityComponent>("Velocity", m_SelectedEntity, [](auto &vc)
                                         { ImGui::DragFloat("Velocity X", &vc.vx, 0.1f); 
                                            ImGui::DragFloat("Velocity Y", &vc.vy, 0.1f);
                                        });

        DrawComponent<CameraComponent>("Camera", m_SelectedEntity, [](auto &cc)
                                       {
                                           ImGui::Checkbox("Primary", &cc.primary);
                                       });

        DrawComponent<CollisionComponent>("Collision", m_SelectedEntity, [](auto &col)
                                          {
            ImGui::DragFloat2("Size", &col.width, 0.1f);
            ImGui::DragFloat2("Offset", &col.width, 0.1f); });
    }

    ImGui::End();
}
