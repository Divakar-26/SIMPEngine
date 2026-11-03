#include "InspectorPanel.h"

void AcceptPayload(const ImGuiPayload *payload, SpriteComponent & sc );

void InspectorPanel::DrawSpriteSection(SIMPEngine::Entity &m_SelectedEntity)
{
    SIMPEngine::Scene *const selectedScene = m_Context->Scene;
    const entt::entity selectedHandle = m_SelectedEntity.GetHandle();

    DrawComponentCard<SpriteComponent>("Sprite", m_SelectedEntity, [this, selectedScene, selectedHandle](auto &sc)
                                       {
                                           ImGui::DragFloat("Width", &sc.width, 0.1f);
                                           ImGui::DragFloat("Height", &sc.height, 0.1f);

                                           if (sc.texture)
                                           {
                                               ImGui::Text("Texture:");
                                               ImGui::Text("%s",
                                                           sc.texture->GetVFSPath().c_str());

                                               ImGui::Dummy(ImVec2(0, 4));
                                               ImGui::Text("Preview");

                                               float previewSize = 128.0f;

                                               float panelWidth = ImGui::GetContentRegionAvail().x;
                                               float cursorX = (panelWidth - previewSize) * 0.5f;

                                               if (cursorX > 0.0f)
                                                   ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cursorX);

                                               ImGui::Image(
                                                   (ImTextureID)sc.texture->GetID(),
                                                   ImVec2(previewSize, previewSize),
                                                   ImVec2(0, 1),
                                                   ImVec2(1, 0));

                                               if (ImGui::BeginDragDropTarget())
                                               {
                                                   if (const ImGuiPayload *payload =
                                                           ImGui::AcceptDragDropPayload("TEXTURE_ASSET"))
                                                   {
                                                       AcceptPayload(payload, sc);
                                                   }

                                                   ImGui::EndDragDropTarget();
                                               }
                                           }
                                           else
                                           {
                                               ImGui::Button("Drop Texture Here");
                                               if (ImGui::BeginDragDropTarget())
                                               {
                                                   if (const ImGuiPayload *payload =
                                                           ImGui::AcceptDragDropPayload("TEXTURE_ASSET"))
                                                   {
                                                       AcceptPayload(payload, sc);
                                                   }

                                                   ImGui::EndDragDropTarget();
                                               }
                                           }

                                           if (ImGui::Button("Select Texture"))
                                           {
                                               m_AssetPicker->Open(
                                                   "Select Texture",
                                                   "assets://",
                                                   [this, selectedScene, selectedHandle](const std::string &vpath)
                                                   {
                                                       if (m_Context->Scene != selectedScene)
                                                           return;
                                                       SIMPEngine::Entity selected(selectedHandle, selectedScene);
                                                       if (!selected || !selected.HasComponent<SpriteComponent>())
                                                           return;

                                                       auto texture =
                                                           std::make_shared<SIMPEngine::Texture>();

                                                       auto real =
                                                           SIMPEngine::VFS::Resolve(vpath);

                                                       if (!real)
                                                       {
                                                           CORE_ERROR(
                                                               "Could not resolve {}",
                                                               vpath);
                                                           return;
                                                       }

                                                       if (!texture->LoadFromFile(real->c_str()))
                                                       {
                                                           CORE_ERROR(
                                                               "Could not load {}",
                                                               *real);
                                                           return;
                                                       }

                                                       texture->SetVFSPath(vpath);

                                                       selected.GetComponent<SpriteComponent>().texture = texture;
                                                   });
                                           }
                                       });
}

void AcceptPayload(const ImGuiPayload *payload, SpriteComponent & sc )
{
    const char *vpath =
        static_cast<const char *>(payload->Data);

    auto texture =
        std::make_shared<SIMPEngine::Texture>();

    auto real =
        SIMPEngine::VFS::Resolve(vpath);

    if (!real)
    {
        CORE_ERROR("RESOLVE FAILED");
        return;
    }

    if (!texture->LoadFromFile(real->c_str()))
    {
        CORE_ERROR("LOAD FAILED");
        return;
    }

    texture->SetVFSPath(vpath);
    sc.texture = texture;
}
