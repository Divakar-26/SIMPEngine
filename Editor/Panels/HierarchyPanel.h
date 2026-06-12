#pragma once

#include <imgui.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Application/Application.h>
#include <Engine/Scene/Scene.h>
#include <../EditorContext.h>

class HierarchyPanel
{
public:
    HierarchyPanel(SIMPEngine::RenderingLayer *renderingLayer, EditorContext* context);

    void OnRender();
    void ShowComponents(SIMPEngine::Entity entity);
    SIMPEngine::Entity * GetSelectedEntity() {return &m_Context->SelectedEntity;}

private:

    EditorContext* m_Context;   
    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;
    char m_EntityNameBuffer[128] = "New Entity";
    bool m_AddTransform = false;
    bool m_AddSprite = false;
    bool m_AddCamera = false;
    bool m_AddCollision = false;
    bool m_OpenAddPopup = false;
    bool m_AddVelocity = false;
    bool m_AddRender = false;
};