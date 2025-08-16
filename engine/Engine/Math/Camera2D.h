#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SIMPEngine
{
    class Camera2D
    {
    public:
        Camera2D(float zoom = 1.0f, const glm::vec2& position = {0.0f, 0.0f});

        void SetPosition(const glm::vec2& position);
        inline const glm::vec2& GetPosition() const { return m_Position; }

        void Move(const glm::vec2& delta);

        void SetZoom(float zoom);
        inline float GetZoom() const { return m_Zoom; }

        void SetRotation(float rotationDegrees);
        inline float GetRotation() const { return m_Rotation; }

        glm::mat4 GetViewMatrix() const;

    private:
        glm::vec2 m_Position;
        float m_Zoom;
        float m_Rotation;

        mutable bool m_Dirty;
        mutable glm::mat4 m_ViewMatrix;

        void RecalculateViewMatrix() const;
    };
}
