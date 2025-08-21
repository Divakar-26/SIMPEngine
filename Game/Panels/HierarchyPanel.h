#pragma once

#include <imgui.h>
#include "Rendering/Renderer.h"
#include "Layer/RenderingLayer.h"
#include "Application/Application.h"
#include"Scene/Scene.h"

class HierarchyPanel
{
public:
    HierarchyPanel(SIMPEngine::RenderingLayer *renderingLayer);

    void OnRender();
    void ShowComponents(entt::entity entity, SIMPEngine::Scene * scene);

private:
    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    
};