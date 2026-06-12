#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Component.h>
#include <../EditorContext.h>

class InspectorPanel
{
public:
    InspectorPanel(EditorContext* m_Context);

    void OnRender();

private:
    EditorContext* m_Context;
};
