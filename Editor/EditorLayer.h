#pragma once
#include <Engine/Layer/Layer.h>
#include <Engine/Core/TimeStep.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Events/Event.h>
#include <Engine/Assets/AssetManager.h>
#include <Engine/Scene/SceneSerializer.h>
#include "Panels/ViewportPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ContentBrowserPanel.h"


class EditorLayer : public SIMPEngine::Layer
{
public:
    EditorLayer(SIMPEngine::RenderingLayer *renderingLayer);
    ~EditorLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(class SIMPEngine::TimeStep ts) override;
    void OnEvent(SIMPEngine::Event &e);
    void OnRender() override;

    void ShowLogs();
    
private:
    ViewportPanel m_ViewportPanel;
    HierarchyPanel m_HieararchyPanel;
    InspectorPanel m_InspectorPanel;
    SIMPEngine::RenderingLayer *m_RenderingLayer;

    std::unique_ptr<ContentBrowserPanel> m_ContentBrowser;
    std::unique_ptr<SIMPEngine::AssetManager> m_AssetManager;

    SIMPEngine::SceneSerializer serializer;

    bool showLogs = false;
};
