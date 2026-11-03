#pragma once

#include <glm/glm.hpp>

class EditorCamera
{
public:
    void SetViewport(float width, float height);
    glm::vec2 GetViewport() const { return {m_Width, m_Height}; }

    glm::mat4 GetView() const;
    glm::mat4 GetProjection() const;

    glm::vec2 ScreenToWorld(const glm::vec2 &pixel) const;
    glm::vec2 WorldToScreen(const glm::vec2 &world) const;

    void Pan(const glm::vec2 &pixelDelta);
    void ZoomAt(float steps, const glm::vec2 &pixel);
    void Frame(const glm::vec2 &center, const glm::vec2 &size);

    void SetPosition(const glm::vec2 &position) { m_Position = position; }
    const glm::vec2 &GetPosition() const { return m_Position; }
    void SetZoom(float zoom);
    float GetZoom() const { return m_Zoom; }

private:
    glm::vec2 m_Position{0.0f};
    float m_Zoom = 0.5f;
    float m_Width = 1.0f;
    float m_Height = 1.0f;
};
