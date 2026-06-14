
#pragma once

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <unordered_set>

class ViewportPanel;

struct EditorContext
{
    SIMPEngine::Scene *Scene = nullptr;

    SIMPEngine::Entity SelectedEntity;
    SIMPEngine::Entity HoveredEntity{};

    ViewportPanel *Viewport = nullptr;
    bool ViewportHovered = false;
    bool ViewportFocused = false;
};