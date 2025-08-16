#include "Camera2D.h"

namespace SIMPEngine
{
    Camera2D::Camera2D(float zoom, const glm::vec2& position)
        : m_Position(position), m_Zoom(zoom), m_Rotation(0.0f),
          m_Dirty(true), m_ViewMatrix(1.0f)
    {
    }

    void Camera2D::SetPosition(const glm::vec2& position)
    {
        m_Position = position;
        m_Dirty = true;
    }

    void Camera2D::Move(const glm::vec2& delta)
    {
        m_Position += delta;
        m_Dirty = true;
    }

    void Camera2D::SetZoom(float zoom)
    {
        m_Zoom = glm::clamp(zoom, 0.1f, 10.0f);
        m_Dirty = true;
    }

    void Camera2D::SetRotation(float rotationDegrees)
    {
        m_Rotation = rotationDegrees;
        m_Dirty = true;
    }

    glm::mat4 Camera2D::GetViewMatrix() const
    {
        if (m_Dirty)
            RecalculateViewMatrix();
        return m_ViewMatrix;
    }

    void Camera2D::RecalculateViewMatrix() const
    {
        glm::mat4 transform = glm::mat4(1.0f);

        // Translate to camera position (negative because we move the world)
        transform = glm::translate(transform, glm::vec3(-m_Position, 0.0f));

        // Apply rotation
        transform = glm::rotate(transform, glm::radians(-m_Rotation), glm::vec3(0.0f, 0.0f, 1.0f));

        // Apply zoom
        transform = glm::scale(transform, glm::vec3(m_Zoom, m_Zoom, 1.0f));

        m_ViewMatrix = transform;
        m_Dirty = false;
    }
}
