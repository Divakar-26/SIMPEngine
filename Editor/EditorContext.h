
#pragma once

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <unordered_set>

class ViewportPanel;

enum class SceneState { Edit, Play, Pause };

struct EditorContext
{
    SIMPEngine::Scene *Scene = nullptr;

    SIMPEngine::Entity SelectedEntity; 
    SIMPEngine::Entity HoveredEntity{};

    SceneState State = SceneState::Edit;

    ViewportPanel *Viewport = nullptr;
    bool ViewportHovered = false;
    bool ViewportFocused = false;
};