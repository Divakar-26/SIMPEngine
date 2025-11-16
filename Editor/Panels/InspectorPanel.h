#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Component.h>

class InspectorPanel
{
public:
    InspectorPanel() = default;

    void SetSelectedEntity(SIMPEngine::Entity entity) { m_SelectedEntity = entity; }
    void OnRender();

private:
    SIMPEngine::Entity m_SelectedEntity;
};
