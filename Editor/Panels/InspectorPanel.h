#pragma once
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Component.h"

class InspectorPanel
{
public:
    InspectorPanel() = default;

    void SetSelectedEntity(SIMPEngine::Entity entity) { m_SelectedEntity = entity; }
    void OnRender();

private:
    SIMPEngine::Entity m_SelectedEntity;
};
