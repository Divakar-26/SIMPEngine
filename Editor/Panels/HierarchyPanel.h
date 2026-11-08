#pragma once

#include <imgui.h>
#include <Engine/Scene/Scene.h>
#include <../EditorContext.h>
#include <../EditorCommands.h>
#include <EditorPanel.h>

#include <string>
#include <vector>

class HierarchyPanel : public EditorPanel
{
public:
    static constexpr const char *WindowId = "###Hierarchy";

    HierarchyPanel(EditorContext* context);

    void OnImGuiRender() override;

private:
    enum class TypeFilter
    {
        All,
        Groups,
        Sprites,
        Shapes,
        Text,
        Tilemaps,
        Particles,
        Lights,
        Glow,
        Cameras,
        Audio,
        Physics,
        Colliders,
        Scripts,
        Triggers,
        Count
    };

    void DrawToolbar();
    void DrawFilterMenu();
    void DrawFlatList(const std::vector<entt::entity> &entities);
    void DrawEntityNode(SIMPEngine::Entity entity);
    void DrawEntityContextMenu(SIMPEngine::Entity entity);
    void DrawRenameField(SIMPEngine::Entity entity, const ImVec2 &min, const ImVec2 &max);
    void HandleRowInput(SIMPEngine::Entity entity, bool clicked, bool doubleClicked);
    void BeginRename(entt::entity handle);
    void TrackSelection();
    bool IsRoot(entt::entity handle);
    bool PassesFilter(entt::entity handle);
    std::string ParentPath(entt::entity handle);
    std::vector<entt::entity> Sorted(std::vector<entt::entity> handles);

    EditorContext* m_Context;
    char m_SearchBuffer[128] = "";

    TypeFilter m_TypeFilter = TypeFilter::All;
    bool m_SortByName = false;
    int m_ForceOpen = -1;

    entt::entity m_LastSelected = entt::null;
    bool m_SelectionFromTree = false;
    std::vector<entt::entity> m_RevealPath;
    entt::entity m_ScrollTarget = entt::null;

    entt::entity m_Renaming = entt::null;
    std::string m_RenameBuffer;
    bool m_RenameFocus = false;
    int m_RenameAge = 0;

    entt::entity m_PendingDelete = entt::null;
    entt::entity m_PendingDuplicate = entt::null;
    entt::entity m_PendingChild = entt::null;
    entt::entity m_PendingParent = entt::null;
    entt::entity m_PendingNewChild = entt::null;
    entt::entity m_PendingUngroup = entt::null;
    bool m_PendingReparent = false;
    bool m_PendingCreate = false;
    bool m_PendingGroup = false;
};
