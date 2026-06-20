#include "ViewportPanel.h"

void ViewportPanel::FocusOnSelectedEntity()
{
    if (!m_Context->SelectedEntity)
        return;

    m_RenderingLayer
        ->GetCamera()
        .FocusOn(
            SIMPEngine::GetEntityCenter(
                m_Context->SelectedEntity));
} 