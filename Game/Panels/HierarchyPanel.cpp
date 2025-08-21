#include "HierarchyPanel.h"
#include "Scene/Scene.h"

entt::entity m_SelectedEntity;

template <typename T>
void DrawComponent(entt::entity entity, SIMPEngine::Scene *scene, const char *name)
{
    if (scene->GetRegistry().any_of<T>(entity))
    {
        bool open = ImGui::TreeNodeEx((void *)typeid(T).hash_code(),
                                      ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth,
                                      "%s", name);

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Remove Component"))
                scene->GetRegistry().remove<T>(entity);

            ImGui::EndPopup();
        }

        if (open)
            ImGui::TreePop();
    }
}

HierarchyPanel::HierarchyPanel(SIMPEngine::RenderingLayer *rl) : m_RenderingLayer(rl) {}

void HierarchyPanel::OnRender()
{
    ImGui::Begin("Panel");
    SIMPEngine::Scene &scene = m_RenderingLayer->GetScene();

    scene.GetRegistry().view<TagComponent>().each([&](auto entity, auto &tag)
                                                  {
        ImGuiTreeNodeFlags flags =  ImGuiTreeNodeFlags_SpanAvailWidth;

        bool opened = ImGui::TreeNodeEx((void*)entity, flags, "%s", tag.Tag.c_str());
        if(ImGui::IsItemClicked()){
            m_SelectedEntity = entity;
        }

        if(opened){
            ShowComponents(m_SelectedEntity, &scene);
            ImGui::TreePop();
        } });

    ImGui::End();
}

void HierarchyPanel::ShowComponents(entt::entity entity, SIMPEngine::Scene *scene)
{
    DrawComponent<TransformComponent>(entity, scene, "Transform");
    DrawComponent<SpriteComponent>(entity, scene, "Sprite");
    DrawComponent<VelocityComponent>(entity, scene, "Velocity");
    DrawComponent<RenderComponent>(entity, scene, "Render");
    DrawComponent<CameraComponent>(entity, scene, "Camera Entity");
    DrawComponent<CollisionComponent>(entity, scene, "Collision Box");
}