#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Component.h>
#include <../EditorContext.h>
#include <../EditorCommands.h>
#include <EditorPanel.h>

class InspectorPanel : public EditorPanel
{
public:
    InspectorPanel(EditorContext* m_Context);

    void OnImGuiRender() override;

private:
    EditorContext* m_Context;
};
