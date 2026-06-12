#pragma once

#include <imgui.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Layer/RenderingLayer.h>
#include <Engine/Application/Application.h>
#include <Engine/Scene/Scene.h>
#include <../EditorContext.h>
#include <../EditorCommands.h>
#include <EditorPanel.h>


class HierarchyPanel : public EditorPanel
{
public:
    HierarchyPanel(EditorContext* context);

    void OnImGuiRender() override;
    void ShowComponents(SIMPEngine::Entity entity);

private:

    EditorContext* m_Context;   
    char m_EntityNameBuffer[128] = "New Entity";
    bool m_AddTransform = false;
    bool m_AddSprite = false;
    bool m_AddCamera = false;
    bool m_AddCollision = false;
    bool m_OpenAddPopup = false;
    bool m_AddVelocity = false;
    bool m_AddRender = false;
};