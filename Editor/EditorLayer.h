#pragma once
#include "Layer/Layer.h"
#include "TimeStep.h"
#include "Panels/ViewportPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Layer/RenderingLayer.h"

class EditorLayer : public SIMPEngine::Layer
{
public:
    EditorLayer(SIMPEngine::RenderingLayer *renderingLayer);
    ~EditorLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(class SIMPEngine::TimeStep ts) override;
    void OnRender() override;

    void ShowLogs();
    
private:
    ViewportPanel m_ViewportPanel;
    HierarchyPanel m_HieararchyPanel;
    bool showLogs = false;
};
