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
    constexpr const char *FilterNames[] = {"Everything", "Groups", "Sprites", "Shapes", "Text", "Tilemaps", "Particles", "Lights", "Glow", "Cameras", "Audio", "Rigid Bodies", "Colliders", "Scripts"};
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

bool HierarchyPanel::PassesFilter(entt::entity handle)
{
    auto &registry = m_Context->Scene->GetRegistry();
    switch (m_TypeFilter)
    {
    case TypeFilter::Groups:
        return EditorCommands::IsGroup(SIMPEngine::Entity(handle, m_Context->Scene));
    case TypeFilter::Sprites:
        return registry.any_of<SpriteComponent, AnimatedSpriteComponent>(handle);
    case TypeFilter::Shapes:
        return registry.all_of<RenderComponent>(handle);
    case TypeFilter::Text:
        return registry.all_of<TextComponent>(handle);
    case TypeFilter::Tilemaps:
        return registry.all_of<TilemapComponent>(handle);
    case TypeFilter::Particles:
        return registry.all_of<ParticleEmitterComponent>(handle);
    case TypeFilter::Lights:
        return registry.all_of<LightComponent>(handle);
    case TypeFilter::Glow:
        return registry.all_of<GlowComponent>(handle);
    case TypeFilter::Cameras:
        return registry.all_of<CameraComponent>(handle);
    case TypeFilter::Audio:
        return registry.all_of<AudioSourceComponent>(handle);
    case TypeFilter::Physics:
        return registry.all_of<PhysicsComponent>(handle);
    case TypeFilter::Colliders:
        return registry.all_of<CollisionComponent>(handle);
    case TypeFilter::Scripts:
        return registry.all_of<ScriptComponent>(handle);
    default:
        return true;
    }
}

std::string HierarchyPanel::ParentPath(entt::entity handle)
{
    auto &registry = m_Context->Scene->GetRegistry();
    std::string path;
    int depth = 0;
    for (auto *h = registry.try_get<HierarchyComponent>(handle); h && h->parent != entt::null && registry.valid(h->parent) && depth < 3; ++depth)
    {
        const std::string &name = registry.get<TagComponent>(h->parent).Tag;
        path = path.empty() ? name : name + " / " + path;
        h = registry.try_get<HierarchyComponent>(h->parent);
    }
    return path;
}

std::vector<entt::entity> HierarchyPanel::Sorted(std::vector<entt::entity> handles)
{
    if (!m_SortByName)
        return handles;
    auto &registry = m_Context->Scene->GetRegistry();
    std::stable_sort(handles.begin(), handles.end(), [&](entt::entity a, entt::entity b)
                     {
        bool ga = EditorCommands::IsGroup(SIMPEngine::Entity(a, m_Context->Scene));
        bool gb = EditorCommands::IsGroup(SIMPEngine::Entity(b, m_Context->Scene));
        if (ga != gb)
            return ga;
        return registry.get<TagComponent>(a).Tag < registry.get<TagComponent>(b).Tag; });
    return handles;
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

void HierarchyPanel::TrackSelection()
{
    auto &registry = m_Context->Scene->GetRegistry();
    entt::entity current = m_Context->SelectedEntity && registry.valid(m_Context->SelectedEntity.GetHandle()) ? m_Context->SelectedEntity.GetHandle() : entt::null;
    if (current == m_LastSelected)
        return;

    m_LastSelected = current;
    if (m_SelectionFromTree || current == entt::null)
    {
        m_SelectionFromTree = false;
        return;
    }

    m_RevealPath.clear();
    for (auto *h = registry.try_get<HierarchyComponent>(current); h && h->parent != entt::null && registry.valid(h->parent);)
    {
        m_RevealPath.push_back(h->parent);
        h = registry.try_get<HierarchyComponent>(h->parent);
    }
    m_ScrollTarget = current;
}

void HierarchyPanel::HandleRowInput(SIMPEngine::Entity entity, bool clicked, bool doubleClicked)
{
    if (clicked)
    {
        m_SelectionFromTree = true;
        if (ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift)
            m_Context->ToggleSelection(entity);
        else
            m_Context->Select(entity);
    }
    if (doubleClicked)
        BeginRename(entity.GetHandle());
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

    TrackSelection();
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
    else if (m_SearchBuffer[0] || m_TypeFilter != TypeFilter::All)
    {
        DrawFlatList(entities);
    }
    else
    {
        std::vector<entt::entity> roots;
        for (auto handle : entities)
            if (IsRoot(handle))
                roots.push_back(handle);
        for (auto handle : Sorted(roots))
            DrawEntityNode(SIMPEngine::Entity(handle, &scene));
    }
    m_ForceOpen = -1;
    m_RevealPath.clear();

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
            if (ImGui::MenuItem("Collapse All"))
                m_ForceOpen = 0;
            if (ImGui::MenuItem("Expand All"))
                m_ForceOpen = 1;
            ImGui::EndPopup();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput && m_Renaming == entt::null)
    {
        if (m_Context->SelectedEntity && ImGui::IsKeyPressed(ImGuiKey_F2, false))
            BeginRename(m_Context->SelectedEntity.GetHandle());
        if (m_Context->SelectedEntity && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_G, false))
        {
            if (ImGui::GetIO().KeyShift)
                m_PendingUngroup = m_Context->SelectedEntity.GetHandle();
            else
                m_PendingGroup = true;
        }
    }

    ImGui::End();

    if (m_PendingCreate)
    {
        m_PendingCreate = false;
        auto entity = EditorCommands::CreateEntity(*m_Context, "Entity");
        m_Context->SelectedEntity = entity;
        BeginRename(entity.GetHandle());
    }

    if (m_PendingGroup)
    {
        m_PendingGroup = false;
        auto group = EditorCommands::GroupSelection(*m_Context);
        if (group)
        {
            m_SelectionFromTree = true;
            BeginRename(group.GetHandle());
        }
    }

    if (m_PendingUngroup != entt::null)
    {
        if (registry.valid(m_PendingUngroup))
        {
            m_SelectionFromTree = true;
            EditorCommands::Ungroup(*m_Context, SIMPEngine::Entity(m_PendingUngroup, &scene));
        }
        m_PendingUngroup = entt::null;
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

    float button = ImGui::GetFrameHeight();
    float buttons = button * 3.0f + 4.0f * 3.0f;
    SearchBox("##search", m_SearchBuffer, sizeof(m_SearchBuffer), "Filter", std::max(60.0f, ImGui::GetContentRegionAvail().x - buttons));

    ImGui::SameLine(0, 4);
    bool filtering = m_TypeFilter != TypeFilter::All || m_SortByName;
    if (IconButton("##typefilter", ICON_LC_LIST_FILTER, "Show only a kind of entity, or sort A to Z", filtering))
        ImGui::OpenPopup("##hierarchyfilter");
    DrawFilterMenu();

    ImGui::SameLine(0, 4);
    if (IconButton("##collapse", ICON_LC_CHEVRONS_DOWN_UP, "Collapse all groups"))
        m_ForceOpen = 0;
    ImGui::SameLine(0, 4);
    if (IconButton("##expand", ICON_LC_CHEVRONS_UP_DOWN, "Expand all groups"))
        m_ForceOpen = 1;
    EndToolbar();
}

void HierarchyPanel::DrawFilterMenu()
{
    using namespace EditorUI;

    if (!ImGui::BeginPopup("##hierarchyfilter"))
        return;

    SectionLabel("Show");
    for (int i = 0; i < static_cast<int>(TypeFilter::Count); ++i)
        if (ImGui::MenuItem(FilterNames[i], nullptr, static_cast<int>(m_TypeFilter) == i))
            m_TypeFilter = static_cast<TypeFilter>(i);
    ImGui::Separator();
    ImGui::MenuItem("Sort A to Z", nullptr, &m_SortByName);
    ImGui::EndPopup();
}

void HierarchyPanel::DrawFlatList(const std::vector<entt::entity> &entities)
{
    using namespace EditorUI;

    auto &registry = m_Context->Scene->GetRegistry();
    std::vector<entt::entity> matches;
    for (auto handle : entities)
    {
        if (!PassesFilter(handle))
            continue;
        if (m_SearchBuffer[0] && !ContainsInsensitive(registry.get<TagComponent>(handle).Tag.c_str(), m_SearchBuffer))
            continue;
        matches.push_back(handle);
    }
    matches = Sorted(matches);

    if (matches.empty())
    {
        EmptyState("No matches", m_TypeFilter != TypeFilter::All ? "Try another kind in the filter menu." : nullptr);
        return;
    }

    char summary[64];
    std::snprintf(summary, sizeof(summary), "%zu %s", matches.size(), matches.size() == 1 ? "match" : "matches");
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextFaint);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::TextUnformatted(summary);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 3));

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(matches.size()));
    while (clipper.Step())
    {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
        {
            entt::entity handle = matches[static_cast<size_t>(i)];
            SIMPEngine::Entity entity(handle, m_Context->Scene);
            const std::string &tag = registry.get<TagComponent>(handle).Tag;
            std::string path = ParentPath(handle);

            Visual v = EntityVisual(entity);
            TreeRowOptions opt;
            opt.Leaf = true;
            opt.Selected = m_Context->IsSelected(handle);
            opt.HideLabel = m_Renaming == handle;
            opt.Trailing = path.empty() ? nullptr : path.c_str();
            char id[32];
            std::snprintf(id, sizeof(id), "##s%u", static_cast<unsigned>(handle));
            auto row = TreeRow(id, v.Icon, v.Color(), tag.empty() ? "(unnamed)" : tag.c_str(), opt);
            HandleRowInput(entity, row.Clicked, row.DoubleClicked);
            if (ImGui::BeginPopupContextItem())
            {
                if (!m_Context->IsSelected(handle))
                    m_Context->Select(entity);
                DrawEntityContextMenu(entity);
                ImGui::EndPopup();
            }
            if (m_Renaming == handle)
                DrawRenameField(entity, row.LabelMin, row.LabelMax);
        }
    }
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

    bool group = EditorCommands::IsGroup(entity);
    Visual v = EntityVisual(entity);
    const char *icon = group ? ICON_LC_FOLDER : v.Icon;
    ImVec4 iconColor = group ? Colors::FolderTint : v.Color();

    char count[16];
    std::snprintf(count, sizeof(count), "%zu", children.size());

    TreeRowOptions opt;
    opt.Leaf = children.empty();
    opt.Selected = m_Context->IsSelected(handle);
    opt.Highlight = m_Context->HoveredEntity && m_Context->HoveredEntity.GetHandle() == handle;
    opt.DefaultOpen = !children.empty() && children.size() <= 6;
    opt.HideLabel = m_Renaming == handle;
    if (!children.empty())
        opt.Trailing = count;
    else if (entity.HasComponent<ScriptComponent>())
        opt.Trailing = ICON_LC_SCROLL_TEXT;

    char id[32];
    std::snprintf(id, sizeof(id), "##e%u", static_cast<unsigned>(handle));
    if (!children.empty())
    {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        bool reveal = std::find(m_RevealPath.begin(), m_RevealPath.end(), handle) != m_RevealPath.end();
        if (m_ForceOpen >= 0 || reveal)
            window->DC.StateStorage->SetBool(window->GetID(id), reveal || m_ForceOpen == 1);
    }

    const std::string &tag = entity.GetComponent<TagComponent>().Tag;
    auto row = TreeRow(id, icon, iconColor, tag.empty() ? "(unnamed)" : tag.c_str(), opt);
    if (m_ScrollTarget == handle)
    {
        ImGui::SetScrollHereY(0.4f);
        m_ScrollTarget = entt::null;
    }

    HandleRowInput(entity, row.Clicked, row.DoubleClicked);

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
        for (auto child : Sorted(children))
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
    if (ImGui::MenuItem(m_Context->HasExtraSelection() ? "Group Selected" : "Group", "Ctrl+G"))
        m_PendingGroup = true;
    auto *hierarchy = entity.GetScene()->GetRegistry().try_get<HierarchyComponent>(entity.GetHandle());
    if (ImGui::MenuItem("Ungroup", "Ctrl+Shift+G", false, hierarchy && !hierarchy->children.empty() && EditorCommands::IsGroup(entity)))
        m_PendingUngroup = entity.GetHandle();

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
