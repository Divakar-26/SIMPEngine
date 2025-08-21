#pragma once

#include <imgui.h>
#include "Rendering/Renderer.h"
#include "Layer/RenderingLayer.h"
#include "Application/Application.h"
#include "Scene/Scene.h"

class HierarchyPanel
{
public:
    HierarchyPanel(SIMPEngine::RenderingLayer *renderingLayer);

    void OnRender();
    void ShowComponents(SIMPEngine::Entity entity);

private:
    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    char m_EntityNameBuffer[128] = "New Entity";
    bool m_AddTransform = false;
    bool m_AddSprite = false;
    bool m_AddCamera = false;
    bool m_AddCollision = false;
    bool m_OpenAddPopup = false;
    bool m_AddVelocity = false;
};