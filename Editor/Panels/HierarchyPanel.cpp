#include "HierarchyPanel.h"
#include "Scene/Scene.h"
#include "PlayerController.h"
#include "../icons.h"

template <typename T>
void DrawComponent(SIMPEngine::Entity entity, const char *name)
{
    if (entity.HasComponent<T>())
    {
        bool open = ImGui::TreeNodeEx((void *)(uint64_t)typeid(T).hash_code(),
                                      ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth,
                                      "%s", name);

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem(ICON_DELETE_LEFT " Remove Component"))
                entity.RemoveComponent<T>();

            ImGui::EndPopup();
        }

        if (open)
            ImGui::TreePop();
    }
}

HierarchyPanel::HierarchyPanel(SIMPEngine::RenderingLayer *rl) : m_RenderingLayer(rl) {}

void HierarchyPanel::OnRender()
{
    ImGui::Begin(ICON_FOLDER "Hierarchy");
    SIMPEngine::Scene &scene = m_RenderingLayer->GetScene();

    if (ImGui::Button("+", ImVec2(40, 40)))
    {
        ImGui::OpenPopup("AddEntityPopup");
    }

    if (ImGui::BeginPopupModal("AddEntityPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Input entity name
        ImGui::InputText("Entity Name", m_EntityNameBuffer, sizeof(m_EntityNameBuffer));

        ImGui::Separator();

        // Component checkboxes
        ImGui::Text("Select Components:");
        ImGui::Checkbox("Sprite", &m_AddSprite);
        ImGui::Checkbox("Camera", &m_AddCamera);
        ImGui::Checkbox("Velocity", &m_AddVelocity);
        ImGui::Checkbox("Collision", &m_AddCollision);
        ImGui::Checkbox("Render", &m_AddRender);

        ImGui::Separator();

        if (ImGui::Button("Add"))
        {
            auto &registry = m_RenderingLayer->GetScene().GetRegistry();

            SIMPEngine::Entity entity = scene.CreateEntity(m_EntityNameBuffer);

            if (m_AddVelocity)
                entity.AddComponent<VelocityComponent>();
            if (m_AddSprite)
                entity.AddComponent<SpriteComponent>(nullptr, 0.0f, 0.0f);
            if (m_AddCamera)
                entity.AddComponent<CameraComponent>(1.0f, glm::vec2(0.0f, 0.0f));
            if (m_AddCollision)
                entity.AddComponent<CollisionComponent>();
            if (m_AddRender)
                entity.AddComponent<RenderComponent>();

            m_AddVelocity = m_AddSprite = m_AddCamera = m_AddCollision = false;
            strcpy(m_EntityNameBuffer, "New Entity");

            // for script
            // auto & script = entity.AddComponent<ScriptComponent>();
            // script.Bind<PlayerController>();

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_MULTIPLY "Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    scene.GetRegistry().view<TagComponent>().each([&](auto handle, auto &tag)
                                                  {
    SIMPEngine::Entity entity(handle, &scene);

    std::string nameWithIcon = std::string(ICON_CUBE) + " " + tag.Tag;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)handle, flags, "%s", nameWithIcon.c_str());

    if (ImGui::IsItemClicked())
        m_SelectedEntity = entity;
              
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::BeginMenu("Add Component"))
        {
            if (!entity.HasComponent<TransformComponent>() && ImGui::MenuItem("Transform"))
                entity.AddComponent<TransformComponent>();

            if (!entity.HasComponent<SpriteComponent>() && ImGui::MenuItem("Sprite"))
                entity.AddComponent<SpriteComponent>(nullptr, 0.0f, 0.0f);

            if (!entity.HasComponent<VelocityComponent>() && ImGui::MenuItem("Velocity"))
                entity.AddComponent<VelocityComponent>();

            if (!entity.HasComponent<RenderComponent>() && ImGui::MenuItem("Render"))
                entity.AddComponent<RenderComponent>();

            if (!entity.HasComponent<CameraComponent>() && ImGui::MenuItem("Camera"))
                entity.AddComponent<CameraComponent>();

            if (!entity.HasComponent<CollisionComponent>() && ImGui::MenuItem("Collision"))
                entity.AddComponent<CollisionComponent>();

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Delete Entity"))
        {
            scene.GetRegistry().destroy(handle);
            if (m_SelectedEntity.GetHandle() == handle)
                m_SelectedEntity = {};
        }

        ImGui::EndPopup();
    }

    if (opened)
    {
        ShowComponents(entity);
        ImGui::TreePop();
    } });

    ImGui::End();
}

void HierarchyPanel::ShowComponents(SIMPEngine::Entity entity)
{
    DrawComponent<TransformComponent>(entity,ICON_ARROWS " Transform");
    DrawComponent<RenderComponent>(entity, ICON_PHOTO_FILM " Render");
    DrawComponent<CameraComponent>(entity, ICON_CAMERA " Camera Entity");
    DrawComponent<CollisionComponent>(entity, ICON_CAR_CRASH " Collision Box");
    DrawComponent<SpriteComponent>(entity, ICON_FACE_SMILE " Sprite");
}
 