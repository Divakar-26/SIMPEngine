#include "Math/Camera2D.h"
#include <glm/gtc/matrix_transform.hpp>

namespace SIMPEngine
{

    Camera2D::Camera2D()
        : m_Position(0.0f, 0.0f), m_Zoom(1.0f), m_ViewMatrixNeedsUpdate(true)
    {
    }

    void Camera2D::SetPosition(const glm::vec2 &position)
    {
        m_Position = position;
        m_ViewMatrixNeedsUpdate = true;
    }

    const glm::vec2 &Camera2D::GetPosition() const
    {
        return m_Position;
    }

    void Camera2D::Move(const glm::vec2 &delta)
    {
        m_Position += delta;
        m_ViewMatrixNeedsUpdate = true;
    }

    void Camera2D::SetZoom(float zoom)
    {
        m_Zoom = zoom;
        m_ViewMatrixNeedsUpdate = true;
    }

    float Camera2D::GetZoom() const
    {
        return m_Zoom;
    }

    void Camera2D::RecalculateViewMatrix() const
    {
        // Create translation matrix (move the world opposite to camera position)
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-m_Position, 0.0f));

        // Create scale matrix for zoom
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(m_Zoom, m_Zoom, 1.0f));

        // View matrix is scale * translation
        m_ViewMatrix = scale * translation;

        m_ViewMatrixNeedsUpdate = false;
    }

    glm::mat4 Camera2D::GetViewMatrix() const
    {
        if (m_ViewMatrixNeedsUpdate)
            RecalculateViewMatrix();

        return m_ViewMatrix;
    }

} // namespace SIMPEngine
