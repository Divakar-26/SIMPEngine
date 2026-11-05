#define IMGUI_DEFINE_MATH_OPERATORS
#include "HierarchyPanel.h"
#include "ComponentMenu.h"
#include "UI/Visuals.h"
#include "UI/Widgets.h"

#include <Engine/Scene/Scene.h>

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <cstdio>
#include <vector>

namespace
{
    constexpr const char *EntityPayload = "HIERARCHY_ENTITY";
}

HierarchyPanel::HierarchyPanel(
    EditorContext *context)
    : m_Context(context)
{
}

bool HierarchyPanel::IsRoot(entt::entity handle)
{
    auto &registry = m_Context->Scene->GetRegistry();
    auto *h = registry.try_get<HierarchyComponent>(handle);
    return !h || h->parent == entt::null || !registry.valid(h->parent);
}

void HierarchyPanel::BeginRename(entt::entity handle)
{
    auto &registry = m_Context->Scene->GetRegistry();
    if (!registry.valid(handle) || !registry.all_of<TagComponent>(handle))
        return;
    m_Renaming = handle;
    m_RenameBuffer = registry.get<TagComponent>(handle).Tag;
    m_RenameFocus = true;
    m_RenameAge = 0;
}

void HierarchyPanel::OnImGuiRender()
{
    using namespace EditorUI;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool visible = ImGui::Begin("Scene###Hierarchy");
    ImGui::PopStyleVar();
    if (!visible)
    {
        ImGui::End();
        return;
    }

    SIMPEngine::Scene &scene = *m_Context->Scene;
    auto &registry = scene.GetRegistry();

    if (m_Context->RenameRequest != entt::null)
    {
        BeginRename(m_Context->RenameRequest);
        m_Context->RenameRequest = entt::null;
    }

    DrawToolbar();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::BeginChild("##tree", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);

    std::vector<entt::entity> entities;
    for (auto handle : registry.view<TagComponent>())
        entities.push_back(handle);
    std::reverse(entities.begin(), entities.end());

    if (entities.empty())
    {
        EmptyState("No entities", "Right-click or press + to add one.");
    }
    else if (m_SearchBuffer[0])
    {
        int shown = 0;
        for (auto handle : entities)
        {
            SIMPEngine::Entity entity(handle, &scene);
            const std::string &tag = entity.GetComponent<TagComponent>().Tag;
            if (!ContainsInsensitive(tag.c_str(), m_SearchBuffer))
                continue;

            Visual v = EntityVisual(entity);
            TreeRowOptions opt;
            opt.Leaf = true;
            opt.Selected = m_Context->IsSelected(handle);
            char id[32];
            std::snprintf(id, sizeof(id), "##s%u", static_cast<unsigned>(handle));
            auto row = TreeRow(id, v.Icon, v.Color(), tag.c_str(), opt);
            if (row.Clicked)
            {
                if (ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift)
                    m_Context->ToggleSelection(entity);
                else
                    m_Context->Select(entity);
            }
            if (ImGui::BeginPopupContextItem())
            {
                DrawEntityContextMenu(entity);
                ImGui::EndPopup();
            }
            ++shown;
        }
        if (shown == 0)
            EmptyState("No matches");
    }
    else
    {
        for (auto handle : entities)
        {
            if (IsRoot(handle))
                DrawEntityNode(SIMPEngine::Entity(handle, &scene));
        }
    }

    ImVec2 rest = ImGui::GetContentRegionAvail();
    if (rest.y > 1.0f)
    {
        ImGui::InvisibleButton("##empty", ImVec2(std::max(1.0f, rest.x), rest.y));
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            m_Context->SelectedEntity = {};
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(EntityPayload))
            {
                m_PendingChild = *(const entt::entity *)payload->Data;
                m_PendingParent = entt::null;
                m_PendingReparent = true;
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::BeginPopupContextItem("##emptyctx"))
        {
            if (ImGui::MenuItem("New Entity"))
                m_PendingCreate = true;
            ImGui::EndPopup();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && m_Context->SelectedEntity &&
        !ImGui::GetIO().WantTextInput && m_Renaming == entt::null)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
            BeginRename(m_Context->SelectedEntity.GetHandle());
    }

    ImGui::End();

    if (m_PendingCreate)
    {
        m_PendingCreate = false;
        auto entity = EditorCommands::CreateEntity(*m_Context, "Entity");
        m_Context->SelectedEntity = entity;
        BeginRename(entity.GetHandle());
    }

    if (m_PendingNewChild != entt::null)
    {
        if (registry.valid(m_PendingNewChild))
        {
            auto child = EditorCommands::CreateChild(*m_Context, SIMPEngine::Entity(m_PendingNewChild, &scene), "Entity");
            m_Context->SelectedEntity = child;
            BeginRename(child.GetHandle());
        }
        m_PendingNewChild = entt::null;
    }

    if (m_PendingReparent)
    {
        EditorCommands::ReparentEntity(*m_Context,
                                       SIMPEngine::Entity(m_PendingChild, &scene),
                                       SIMPEngine::Entity(m_PendingParent, &scene));
        m_PendingReparent = false;
    }

    if (m_PendingDuplicate != entt::null)
    {
        if (registry.valid(m_PendingDuplicate))
        {
            if (m_Context->HasExtraSelection() && m_Context->IsSelected(m_PendingDuplicate))
                EditorCommands::DuplicateSelection(*m_Context);
            else
                m_Context->Select(EditorCommands::DuplicateEntity(*m_Context, SIMPEngine::Entity(m_PendingDuplicate, &scene)));
        }
        m_PendingDuplicate = entt::null;
    }

    if (m_PendingDelete != entt::null)
    {
        if (registry.valid(m_PendingDelete))
        {
            if (m_Context->HasExtraSelection() && m_Context->IsSelected(m_PendingDelete))
                EditorCommands::DeleteSelection(*m_Context);
            else
                EditorCommands::DeleteEntity(*m_Context, SIMPEngine::Entity(m_PendingDelete, &scene));
        }
        m_PendingDelete = entt::null;
    }
}

void HierarchyPanel::DrawToolbar()
{
    using namespace EditorUI;

    BeginToolbar("##hierarchy_toolbar");
    if (IconButton("##add", ICON_LC_PLUS, "New entity"))
        m_PendingCreate = true;
    ImGui::SameLine(0, 4);
    SearchBox("##search", m_SearchBuffer, sizeof(m_SearchBuffer), "Filter");
    EndToolbar();
}

void HierarchyPanel::DrawRenameField(SIMPEngine::Entity entity, const ImVec2 &min, const ImVec2 &max)
{
    ImVec2 restore = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(min);
    ImGui::SetNextItemWidth(max.x - min.x);
    bool focusNow = m_RenameFocus;
    if (focusNow)
    {
        ImGui::SetKeyboardFocusHere();
        m_RenameFocus = false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, std::max(0.0f, (max.y - min.y - ImGui::GetFontSize()) * 0.5f)));
    bool done = ImGui::InputText("##rename", &m_RenameBuffer, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    ImGui::PopStyleVar();

    bool cancel = ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape);
    ++m_RenameAge;
    bool lostFocus = m_RenameAge > 3 && !ImGui::IsItemActive();

    if (cancel)
    {
        m_Renaming = entt::null;
    }
    else if (done || lostFocus)
    {
        if (!m_RenameBuffer.empty())
            EditorCommands::RenameEntity(*m_Context, entity, m_RenameBuffer);
        m_Renaming = entt::null;
    }

    ImGui::SetCursorScreenPos(restore);
    ImGui::Dummy(ImVec2(0, 0));
}

void HierarchyPanel::DrawEntityNode(SIMPEngine::Entity entity)
{
    using namespace EditorUI;

    auto &registry = m_Context->Scene->GetRegistry();
    entt::entity handle = entity.GetHandle();

    std::vector<entt::entity> children;
    if (auto *h = registry.try_get<HierarchyComponent>(handle))
    {
        for (auto child : h->children)
            if (registry.valid(child) && registry.all_of<TagComponent>(child))
                children.push_back(child);
    }

    Visual v = EntityVisual(entity);
    TreeRowOptions opt;
    opt.Leaf = children.empty();
    opt.Selected = m_Context->IsSelected(handle);
    opt.Highlight = m_Context->HoveredEntity && m_Context->HoveredEntity.GetHandle() == handle;
    opt.DefaultOpen = true;
    opt.HideLabel = m_Renaming == handle;
    if (entity.HasComponent<ScriptComponent>())
        opt.Trailing = ICON_LC_SCROLL_TEXT;

    char id[32];
    std::snprintf(id, sizeof(id), "##e%u", static_cast<unsigned>(handle));
    const std::string &tag = entity.GetComponent<TagComponent>().Tag;
    auto row = TreeRow(id, v.Icon, v.Color(), tag.empty() ? "(unnamed)" : tag.c_str(), opt);

    if (row.Clicked)
    {
        if (ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift)
            m_Context->ToggleSelection(entity);
        else
            m_Context->Select(entity);
    }
    if (row.DoubleClicked)
        BeginRename(handle);

    if (m_Renaming != handle && ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload(EntityPayload, &handle, sizeof(handle));
        ImGui::TextUnformatted(tag.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(EntityPayload))
        {
            m_PendingChild = *(const entt::entity *)payload->Data;
            m_PendingParent = handle;
            m_PendingReparent = true;
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (!m_Context->IsSelected(handle))
            m_Context->Select(entity);
        DrawEntityContextMenu(entity);
        ImGui::EndPopup();
    }

    if (m_Renaming == handle)
        DrawRenameField(entity, row.LabelMin, row.LabelMax);

    if (row.Open)
    {
        for (auto child : children)
            DrawEntityNode(SIMPEngine::Entity(child, m_Context->Scene));
        TreeRowPop();
    }
}

void HierarchyPanel::DrawEntityContextMenu(SIMPEngine::Entity entity)
{
    if (ImGui::MenuItem("Rename", "F2"))
        BeginRename(entity.GetHandle());
    if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
        m_PendingDuplicate = entity.GetHandle();
    if (ImGui::MenuItem("Add Child Entity"))
        m_PendingNewChild = entity.GetHandle();

    if (!IsRoot(entity.GetHandle()) && ImGui::MenuItem("Move to Root"))
    {
        m_PendingChild = entity.GetHandle();
        m_PendingParent = entt::null;
        m_PendingReparent = true;
    }

    ImGui::Separator();
    if (ImGui::BeginMenu("Add Component"))
    {
        DrawAddComponentItems(*m_Context, entity);
        ImGui::EndMenu();
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Delete", "Del"))
        m_PendingDelete = entity.GetHandle();

    }
