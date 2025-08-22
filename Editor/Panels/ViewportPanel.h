#pragma once

#include <imgui.h>
#include "Rendering/Renderer.h"
#include "Layer/RenderingLayer.h"
#include "Application/Application.h"


class ViewportPanel
{
public:
    ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer);

    void OnAttach();
    void OnRender();

private:
    SIMPEngine::RenderingLayer *m_RenderingLayer = nullptr;

    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;
};