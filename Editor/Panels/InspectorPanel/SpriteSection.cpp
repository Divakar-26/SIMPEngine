#include "InspectorPanel.h"

void AcceptPayload(const ImGuiPayload *payload, SpriteComponent & sc );

void InspectorPanel::DrawSpriteSection(SIMPEngine::Entity &m_SelectedEntity)
{
    DrawComponentCard<SpriteComponent>("Sprite", m_SelectedEntity, [this](auto &sc)
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
                                                   CORE_ERROR("TARGET HOVERED");
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
                                                   [&](const std::string &vpath)
                                                   {
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

                                                       sc.texture = texture;
                                                   });
                                           }

                                           // later:
                                           // drag-drop texture assignment
                                       });
}

void AcceptPayload(const ImGuiPayload *payload, SpriteComponent & sc )
{
    CORE_ERROR("PAYLOAD ACCEPTED");

    const char *vpath =
        static_cast<const char *>(payload->Data);

    CORE_ERROR("VPATH = {}", vpath);

    auto texture =
        std::make_shared<SIMPEngine::Texture>();

    auto real =
        SIMPEngine::VFS::Resolve(vpath);

    if (!real)
    {
        CORE_ERROR("RESOLVE FAILED");
        return;
    }

    CORE_ERROR("REAL PATH = {}", *real);

    if (!texture->LoadFromFile(real->c_str()))
    {
        CORE_ERROR("LOAD FAILED");
        return;
    }

    CORE_ERROR("LOAD SUCCEEDED");

    texture->SetVFSPath(vpath);
    sc.texture = texture;

    CORE_ERROR("TEXTURE ASSIGNED");
}