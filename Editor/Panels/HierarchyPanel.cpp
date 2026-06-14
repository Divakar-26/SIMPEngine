#include "HierarchyPanel.h"
#include <Engine/Scene/Scene.h>
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
            if (ImGui::MenuItem("Remove Component"))
                entity.RemoveComponent<T>();

            ImGui::EndPopup();
        }

        if (open)
            ImGui::TreePop();
    }
}

HierarchyPanel::HierarchyPanel(
    EditorContext *context)
    : m_Context(context)
{
}

void HierarchyPanel::OnImGuiRender()
{
    ImGui::Begin("Hierarchy");
    SIMPEngine::Scene &scene = *m_Context->Scene;

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
        ImGui::Checkbox("Script", &m_AddScript);
        ImGui::Checkbox("Physics", &m_AddPhysics);
        ImGui::Checkbox("Animated Sprite", &m_AddAnimatedSprite);
        ImGui::Checkbox("Lifetime", &m_AddLifetime);
        ImGui::Checkbox("Tilemap", &m_AddTilemap);

        ImGui::Separator();

        if (ImGui::Button("Add"))
        {
            auto &registry = m_Context->Scene->GetRegistry();

            SIMPEngine::Entity entity =
                EditorCommands::CreateEntity(
                    *m_Context,
                    m_EntityNameBuffer);

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
            if (m_AddScript)
                //
                if (m_AddPhysics)
                    entity.AddComponent<PhysicsComponent>();
            if (m_AddAnimatedSprite)
                entity.AddComponent<AnimatedSpriteComponent>();
            if (m_AddLifetime)
                entity.AddComponent<LifetimeComponent>();
            if (m_AddTilemap)
                entity.AddComponent<TilemapComponent>();

            m_AddVelocity = m_AddSprite = m_AddCamera = m_AddCollision = false;
            strcpy(m_EntityNameBuffer, "New Entity");

            // for script

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    scene.GetRegistry().view<TagComponent>().each([&](auto handle, auto &tag)
                                                  {
    SIMPEngine::Entity entity(handle, &scene);

    std::string nameWithIcon = tag.Tag;
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)handle, flags, "%s", nameWithIcon.c_str());

    if (ImGui::IsItemClicked())
        m_Context->SelectedEntity = entity;
              
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

                
                if (!entity.HasComponent<PhysicsComponent>() && ImGui::MenuItem("Physics"))
                entity.AddComponent<PhysicsComponent>();
                
                if (!entity.HasComponent<AnimatedSpriteComponent>() && ImGui::MenuItem("Animated Sprite"))
                entity.AddComponent<AnimatedSpriteComponent>();
                
                if (!entity.HasComponent<LifetimeComponent>() && ImGui::MenuItem("Lifetime"))
                entity.AddComponent<LifetimeComponent>();

                // if (!entity.HasComponent<ScriptComponent>() && ImGui::MenuItem("Script"))
                //     entity.AddComponent<ScriptComponent>();


            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Delete Entity"))
        {
            EditorCommands::DeleteEntity(*m_Context, entity);
        }

        if(ImGui::MenuItem("Add Script")){

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
    DrawComponent<TransformComponent>(entity, "Transform");
    DrawComponent<RenderComponent>(entity, "Render");
    DrawComponent<CameraComponent>(entity, "Camera Entity");
    DrawComponent<CollisionComponent>(entity, "Collision Box");
    DrawComponent<SpriteComponent>(entity, "Sprite");
}
