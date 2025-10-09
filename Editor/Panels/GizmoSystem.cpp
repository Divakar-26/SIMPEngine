#include "GizmoSystem.h"
#include <imgui.h>
#include <float.h>

GizmoHandle GizmoSystem::Update(const glm::vec2& mouseWorldPos, 
                               const glm::vec2& entityPos,
                               const glm::vec2& entitySize,
                               float rotation) {
    glm::vec2 corners[4];
    
    corners[0] = entityPos;
    corners[1] = entityPos + glm::vec2(entitySize.x, 0);
    corners[2] = entityPos + glm::vec2(0, entitySize.y);
    corners[3] = entityPos + entitySize;
    
    return m_HoveredGizmo;
}

GizmoHandle GizmoSystem::DrawHandles(const glm::vec2 corners[4], 
                                    const std::function<glm::vec2(const glm::vec2&)>& worldToScreenFunc) {
    GizmoHandle hoveredGizmo = GizmoHandle::None;
    float minDist = FLT_MAX;
    
    DrawSingleGizmo(corners[0], GizmoHandle::TopLeft, hoveredGizmo, minDist, worldToScreenFunc);
    DrawSingleGizmo(corners[1], GizmoHandle::TopRight, hoveredGizmo, minDist, worldToScreenFunc);
    DrawSingleGizmo(corners[2], GizmoHandle::BottomLeft, hoveredGizmo, minDist, worldToScreenFunc);
    DrawSingleGizmo(corners[3], GizmoHandle::BottomRight, hoveredGizmo, minDist, worldToScreenFunc);
    
    glm::vec2 topCenter = (corners[0] + corners[1]) * 0.5f;
    glm::vec2 bottomCenter = (corners[2] + corners[3]) * 0.5f;
    glm::vec2 leftCenter = (corners[0] + corners[2]) * 0.5f;
    glm::vec2 rightCenter = (corners[1] + corners[3]) * 0.5f;
    
    DrawSingleGizmo(topCenter, GizmoHandle::TopCenter, hoveredGizmo, minDist, worldToScreenFunc);
    DrawSingleGizmo(bottomCenter, GizmoHandle::BottomCenter, hoveredGizmo, minDist, worldToScreenFunc);
    DrawSingleGizmo(leftCenter, GizmoHandle::LeftCenter, hoveredGizmo, minDist, worldToScreenFunc);
    DrawSingleGizmo(rightCenter, GizmoHandle::RightCenter, hoveredGizmo, minDist, worldToScreenFunc);
    
    m_HoveredGizmo = hoveredGizmo;
    return hoveredGizmo;
}

void GizmoSystem::DrawSingleGizmo(const glm::vec2& worldPos, 
                                 GizmoHandle handleType,
                                 GizmoHandle& hoveredGizmo, 
                                 float& minDist,
                                 const std::function<glm::vec2(const glm::vec2&)>& worldToScreenFunc) {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    glm::vec2 screenPos = worldToScreenFunc(worldPos);
    
    ImVec2 mouseScreen = ImGui::GetMousePos();
    glm::vec2 mousePosVec2 = {mouseScreen.x, mouseScreen.y};
    
    float dist = glm::distance(mousePosVec2, screenPos);
    
    bool isHovered = (dist <= m_Style.HitRadius);  
    bool isActive = (m_ActiveGizmo == handleType);
    
    ImU32 color = m_Style.NormalColor;
    if (isActive) color = m_Style.ActiveColor;
    else if (isHovered) color = m_Style.HoverColor;
    
    drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), m_Style.Radius, color);
    drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), m_Style.Radius, IM_COL32(0, 0, 0, 255), 12, 1.0f);
    
    if (isHovered) {
        float hoverRadius = m_Style.Radius * m_Style.HoverRadiusMultiplier;
        drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), hoverRadius,
                           IM_COL32(255, 255, 255, 100), 12, 2.0f);
    }
    
    if (isHovered && dist < minDist) {
        hoveredGizmo = handleType;
        minDist = dist;
    }
}

void GizmoSystem::OnMouseClick(bool clicked) {
    if (clicked && m_HoveredGizmo != GizmoHandle::None) {
        m_ActiveGizmo = m_HoveredGizmo;
        m_IsDragging = true;
    }
}

void GizmoSystem::OnMouseRelease() {
    m_ActiveGizmo = GizmoHandle::None;
    m_IsDragging = false;
}

GizmoSystem::TransformDelta GizmoSystem::CalculateTransformDelta(GizmoHandle gizmo, 
                                                                const glm::vec2& mouseDelta,
                                                                const glm::vec2& entitySize) {
    TransformDelta delta;
    
    switch (gizmo) {
        case GizmoHandle::TopLeft:
            delta.positionDelta = mouseDelta;
            delta.scaleDelta = -mouseDelta / entitySize;
            break;
            
        case GizmoHandle::TopRight:
            delta.positionDelta.y = mouseDelta.y;
            delta.scaleDelta = glm::vec2(mouseDelta.x / entitySize.x, -mouseDelta.y / entitySize.y);
            break;
            
        case GizmoHandle::BottomLeft:
            delta.positionDelta.x = mouseDelta.x;
            delta.scaleDelta = glm::vec2(-mouseDelta.x / entitySize.x, mouseDelta.y / entitySize.y);
            break;
            
        case GizmoHandle::BottomRight:
            delta.scaleDelta = mouseDelta / entitySize;
            break;
            
        case GizmoHandle::TopCenter:
            delta.positionDelta.y = mouseDelta.y;
            delta.scaleDelta.y = -mouseDelta.y / entitySize.y;
            break;
            
        case GizmoHandle::BottomCenter:
            delta.scaleDelta.y = mouseDelta.y / entitySize.y;
            break;
            
        case GizmoHandle::LeftCenter:
            delta.positionDelta.x = mouseDelta.x;
            delta.scaleDelta.x = -mouseDelta.x / entitySize.x;
            break;
            
        case GizmoHandle::RightCenter:
            delta.scaleDelta.x = mouseDelta.x / entitySize.x;
            break;
            
        default:
            break;
    }
    
    return delta;
}