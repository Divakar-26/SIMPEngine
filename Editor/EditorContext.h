#pragma once

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <string>

class ViewportPanel;
class EditorHistory;

enum class SceneState { Edit, Play, Pause };

enum class GizmoTool { Select, Move, Rotate, Scale };

struct EditorContext
{
    SIMPEngine::Scene *Scene = nullptr;

    SIMPEngine::Entity SelectedEntity;
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

    bool IsSelected(entt::entity handle) const
    {
        return SelectedEntity && SelectedEntity.GetHandle() == handle;
    }
};
