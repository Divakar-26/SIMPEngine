#pragma once
#include <glm/glm.hpp>
#include <functional>
#include <vector>
#include <imgui.h>

enum class GizmoHandle {
    None = 0,
    TopLeft, TopRight, BottomLeft, BottomRight,
    TopCenter, BottomCenter, LeftCenter, RightCenter,
    Rotate
};

struct GizmoStyle {
    float Radius = 8.0f;
    float HitRadius = 12.0f;
    ImU32 NormalColor = IM_COL32(255, 255, 0, 255);
    ImU32 HoverColor = IM_COL32(255, 0, 0, 255);
    ImU32 ActiveColor = IM_COL32(0, 255, 0, 255);
    float HoverRadiusMultiplier = 1.8f;
};

class GizmoSystem {
public:
    GizmoSystem() = default;
    
    GizmoHandle Update(const glm::vec2& mouseWorldPos, 
                       const glm::vec2& entityPos,
                       const glm::vec2& entitySize,
                       float rotation = 0.0f);
    
    GizmoHandle DrawHandles(const glm::vec2 corners[4], 
                           const std::function<glm::vec2(const glm::vec2&)>& worldToScreenFunc);
    
    bool IsInteracting() const { return m_ActiveGizmo != GizmoHandle::None; }
    bool IsDragging() const { return m_IsDragging; }
    
    void OnMouseClick(bool clicked);
    void OnMouseRelease();
    
    void SetStyle(const GizmoStyle& style) { m_Style = style; }
    const GizmoStyle& GetStyle() const { return m_Style; }
    
    GizmoHandle GetActiveGizmo() const { return m_ActiveGizmo; }
    
    struct TransformDelta {
        glm::vec2 positionDelta{0.0f};
        glm::vec2 scaleDelta{0.0f};
        float rotationDelta{0.0f};
    };
    
    TransformDelta CalculateTransformDelta(GizmoHandle gizmo, 
                                          const glm::vec2& mouseDelta,
                                          const glm::vec2& entitySize);

private:
    GizmoHandle m_ActiveGizmo = GizmoHandle::None;
    GizmoHandle m_HoveredGizmo = GizmoHandle::None;
    bool m_IsDragging = false;
    glm::vec2 m_DragStartPos{0.0f};
    GizmoStyle m_Style;
    
    void DrawSingleGizmo(const glm::vec2& screenPos, 
                        GizmoHandle handleType,
                        GizmoHandle& hoveredGizmo, 
                        float& minDist,
                        const std::function<glm::vec2(const glm::vec2&)>& worldToScreenFunc);
    
    glm::vec2 CalculateHandleWorldPos(GizmoHandle handle, 
                                     const glm::vec2 corners[4]) const;
};