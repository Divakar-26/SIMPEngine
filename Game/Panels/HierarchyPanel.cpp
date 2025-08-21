#include "HierarchyPanel.h"
#include "Scene/Scene.h"

entt::entity m_SelectedEntity;

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

    if (scene->GetRegistry().any_of<TransformComponent>(entity))
    {
        if (ImGui::TreeNodeEx((void *)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth, "Transform"))
        {
            ImGui::TreePop();
        }
    }

    if (scene->GetRegistry().any_of<SpriteComponent>(entity))
    {
        if (ImGui::TreeNodeEx((void *)typeid(SpriteComponent).hash_code(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth, "Sprite"))
        {
            ImGui::TreePop();
        }
    }

    if (scene->GetRegistry().any_of<VelocityComponent>(entity))
    {
        if (ImGui::TreeNodeEx((void *)typeid(VelocityComponent).hash_code(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth, "Velocity"))
        {
            ImGui::TreePop();
        }
    }

    if (scene->GetRegistry().any_of<RenderComponent>(entity))
    {
        if (ImGui::TreeNodeEx((void *)typeid(RenderComponent).hash_code(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth, "Render"))
        {
            ImGui::TreePop();
        }
    }

    if (scene->GetRegistry().any_of<CameraComponent>(entity))
    {
        if (ImGui::TreeNodeEx((void *)typeid(CameraComponent).hash_code(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth, "Camera Entity"))
        {
            ImGui::TreePop();
        }
    }

    if (scene->GetRegistry().any_of<CollisionComponent>(entity))
    {
        if (ImGui::TreeNodeEx((void *)typeid(CollisionComponent).hash_code(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth, "Collision Box"))
        {
            ImGui::TreePop();
        }
    }
}