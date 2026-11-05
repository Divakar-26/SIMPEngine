#pragma once

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Component.h>

#include <algorithm>
#include <string>
#include <vector>

class ViewportPanel;
class EditorHistory;

enum class SceneState { Edit, Play, Pause };

enum class GizmoTool { Select, Move, Rotate, Scale };

struct EditorContext
{
    SIMPEngine::Scene *Scene = nullptr;

    SIMPEngine::Entity SelectedEntity;
    std::vector<entt::entity> ExtraSelection;
    entt::entity ExtraSelectionOwner = entt::null;
    SIMPEngine::Entity HoveredEntity{};

    SceneState State = SceneState::Edit;
    bool StepRequested = false;

    GizmoTool Tool = GizmoTool::Move;
    EditorHistory *History = nullptr;

    std::string ScenePath;
    bool Dirty = false;

    entt::entity RenameRequest = entt::null;
    bool FocusGameView = false;
    bool FocusSceneView = false;

    ViewportPanel *Viewport = nullptr;
    bool ViewportHovered = false;
    bool ViewportFocused = false;

    bool IsEditing() const { return State == SceneState::Edit; }

    bool HasExtraSelection() const
    {
        return SelectedEntity && !ExtraSelection.empty() && ExtraSelectionOwner == SelectedEntity.GetHandle();
    }

    bool IsSelected(entt::entity handle) const
    {
        if (SelectedEntity && SelectedEntity.GetHandle() == handle)
            return true;
        return HasExtraSelection() && std::find(ExtraSelection.begin(), ExtraSelection.end(), handle) != ExtraSelection.end();
    }

    std::vector<entt::entity> SelectedHandles() const
    {
        std::vector<entt::entity> handles;
        if (!Scene)
            return handles;

        auto &registry = Scene->GetRegistry();
        if (SelectedEntity && registry.valid(SelectedEntity.GetHandle()))
            handles.push_back(SelectedEntity.GetHandle());
        if (HasExtraSelection())
        {
            for (entt::entity handle : ExtraSelection)
            {
                if (registry.valid(handle) && std::find(handles.begin(), handles.end(), handle) == handles.end())
                    handles.push_back(handle);
            }
        }
        return handles;
    }

    std::vector<entt::entity> SelectedRoots() const
    {
        std::vector<entt::entity> all = SelectedHandles();
        std::vector<entt::entity> roots;
        if (!Scene)
            return roots;

        auto &registry = Scene->GetRegistry();
        for (entt::entity handle : all)
        {
            bool covered = false;
            const auto *hierarchy = registry.try_get<HierarchyComponent>(handle);
            entt::entity parent = hierarchy ? hierarchy->parent : entt::null;
            for (int depth = 0; parent != entt::null && registry.valid(parent) && !covered && depth < 256; ++depth)
            {
                covered = std::find(all.begin(), all.end(), parent) != all.end();
                const auto *parentHierarchy = registry.try_get<HierarchyComponent>(parent);
                parent = parentHierarchy ? parentHierarchy->parent : entt::null;
            }
            if (!covered)
                roots.push_back(handle);
        }
        return roots;
    }

    void Select(SIMPEngine::Entity entity)
    {
        SelectedEntity = entity;
        ExtraSelection.clear();
        ExtraSelectionOwner = entt::null;
    }

    void ClearSelection()
    {
        Select(SIMPEngine::Entity{});
    }

    void SetSelection(const std::vector<entt::entity> &handles)
    {
        ExtraSelection.clear();
        ExtraSelectionOwner = entt::null;
        if (handles.empty() || !Scene)
        {
            SelectedEntity = {};
            return;
        }

        SelectedEntity = SIMPEngine::Entity(handles.front(), Scene);
        ExtraSelectionOwner = handles.front();
        ExtraSelection.assign(handles.begin() + 1, handles.end());
    }

    void ToggleSelection(SIMPEngine::Entity entity)
    {
        if (!entity || !Scene)
            return;

        std::vector<entt::entity> handles = SelectedHandles();
        auto it = std::find(handles.begin(), handles.end(), entity.GetHandle());
        if (it != handles.end())
            handles.erase(it);
        else
            handles.insert(handles.begin(), entity.GetHandle());
        SetSelection(handles);
    }

    void MakePrimary(SIMPEngine::Entity entity)
    {
        if (!entity || !IsSelected(entity.GetHandle()))
            return;

        std::vector<entt::entity> handles = SelectedHandles();
        auto it = std::find(handles.begin(), handles.end(), entity.GetHandle());
        if (it != handles.end() && it != handles.begin())
            std::rotate(handles.begin(), it, it + 1);
        SetSelection(handles);
    }
};
