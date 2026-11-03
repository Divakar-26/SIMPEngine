#pragma once

#include <imgui.h>
#include <Engine/Scene/Scene.h>
#include <../EditorContext.h>
#include <../EditorCommands.h>
#include <EditorPanel.h>

#include <string>

class HierarchyPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Hierarchy";

    HierarchyPanel(EditorContext* context);

    void OnImGuiRender() override;

private:
    void DrawToolbar();
    void DrawEntityNode(SIMPEngine::Entity entity);
    void DrawEntityContextMenu(SIMPEngine::Entity entity);
    void DrawRenameField(SIMPEngine::Entity entity, const ImVec2 &min, const ImVec2 &max);
    void BeginRename(entt::entity handle);
    bool IsRoot(entt::entity handle);

    EditorContext* m_Context;
    char m_SearchBuffer[128] = "";

    entt::entity m_Renaming = entt::null;
    std::string m_RenameBuffer;
    bool m_RenameFocus = false;
    int m_RenameAge = 0;

    entt::entity m_PendingDelete = entt::null;
    entt::entity m_PendingDuplicate = entt::null;
    entt::entity m_PendingChild = entt::null;
    entt::entity m_PendingParent = entt::null;
    entt::entity m_PendingNewChild = entt::null;
    bool m_PendingReparent = false;
    bool m_PendingCreate = false;
};
