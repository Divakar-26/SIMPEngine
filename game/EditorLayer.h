#pragma once
#include "Layer/Layer.h"
#include"TimeStep.h"
#include "Panels/ViewportPanel.h"
#include "Layer/RenderingLayer.h"

class EditorLayer : public SIMPEngine::Layer{
public:
    EditorLayer(SIMPEngine::RenderingLayer* renderingLayer);
    ~EditorLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(class SIMPEngine::TimeStep ts) override;
    void OnRender() override;

private:
    ViewportPanel m_ViewportPanel;
};
