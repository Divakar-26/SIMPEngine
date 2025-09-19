#include "Camera2D.h"
#include "Events/EventDispatcher.h"
#include "Events/KeyEvent.h"
#include <SDL3/SDL_keycode.h>
#include "Core/Log.h"
#include "Input/Input.h"
#include "Input/SIMP_Keys.h"

namespace SIMPEngine
{
    Camera2D::Camera2D(float zoom, const glm::vec2 &position) : m_Position(position),
                                                                m_TargetPosition(position),
                                                                m_ManualZoom(zoom),
                                                                m_Rotation(0.0f),
                                                                m_SmoothFactor(10.0f),
                                                                m_MoveSpeed(200.0f),
                                                                m_ZoomSpeed(15.0f),
                                                                m_Dirty(true),
                                                                m_ViewMatrix(1.0f)
    {
    }

    void Camera2D::SetPosition(const glm::vec2 &position)
    {
        m_Position = position;
        m_TargetPosition = position;
        m_Dirty = true;
    }

    void Camera2D::Move(const glm::vec2 &delta)
    {
        m_TargetPosition += delta;
        m_Dirty = true;
    }

    void Camera2D::SetZoom(float zoom)
    {
        m_ManualZoom = glm::clamp(zoom, 0.1f, 10.0f);
        m_Dirty = true;
    }

    void Camera2D::SetRotation(float rotationDegrees)
    {
        m_Rotation = rotationDegrees;
        m_Dirty = true;
    }

    void Camera2D::OnEvent(Event &e)
    {
    }

    void Camera2D::Update(float deltaTime)
    {
        m_Position += (m_TargetPosition - m_Position) * glm::clamp(m_SmoothFactor * deltaTime, 0.0f, 1.0f);

        glm::vec2 delta(0.0f);
        if (Input::IsKeyPressed(SIMPK_A))
            delta.x -= m_MoveSpeed * deltaTime;
        if (Input::IsKeyPressed(SIMPK_D))
            delta.x += m_MoveSpeed * deltaTime;
        if (Input::IsKeyPressed(SIMPK_W))
            delta.y -= m_MoveSpeed * deltaTime;
        if (Input::IsKeyPressed(SIMPK_S))
            delta.y += m_MoveSpeed * deltaTime;

        int wheelDelta = Input::GetMouseWheel();
        if (wheelDelta > 0)
            m_ManualZoom += 0.1f * wheelDelta;
        else if (wheelDelta < 0)
            m_ManualZoom -= 0.1f * -wheelDelta;
        Move(delta);

        // --- Middle Mouse Panning ---
        if (Input::IsMouseButtonPressed(SIMPK_MOUSE_MIDDLE))
        {
            glm::vec2 mousePos(Input::GetMousePosition().first, Input::GetMousePosition().second);

            if (!m_Panning)
            {
                m_Panning = true;
                m_LastMousePos = mousePos;
            }

            glm::vec2 delta = mousePos - m_LastMousePos;
            m_LastMousePos = mousePos;

            delta /= m_ManualZoom;

            Move(-delta);
        }
        else
        {
            m_Panning = false;
        }

        m_ManualZoom = glm::clamp(m_ManualZoom, 0.1f, 10.0f);

        // CORE_ERROR("{} {} {}", m_Position.x , m_Position.y, m_ManualZoom);
    }

    glm::mat4 Camera2D::GetViewMatrix() const
    {
        if (m_Dirty)
            RecalculateViewMatrix();
        return m_ViewMatrix;
    }
    void Camera2D::RecalculateViewMatrix() const
    {
        float totalZoom = m_BaseZoom * m_ManualZoom;

        float halfWidth = m_ViewportWidth / 2.0f;
        float halfHeight = m_ViewportHeight / 2.0f;

        glm::mat4 transform(1.0f);
        if (!isCentered)
        {
            // Origin = top-left
            transform = glm::translate(transform, glm::vec3(halfWidth, halfHeight, 0.0f));
            transform = glm::scale(transform, glm::vec3(totalZoom, totalZoom, 1.0f));
            transform = glm::translate(transform, glm::vec3(-m_Position.x, -m_Position.y, 0.0f));
            transform = glm::rotate(transform, glm::radians(-m_Rotation), glm::vec3(0, 0, 1));
        }
        else
        {
            // Origin = center
            transform = glm::translate(transform, glm::vec3(-m_Position.x, -m_Position.y, 0.0f));
            transform = glm::rotate(transform, glm::radians(-m_Rotation), glm::vec3(0, 0, 1));
            transform = glm::scale(transform, glm::vec3(totalZoom, totalZoom, 1.0f));
        }

        m_ViewMatrix = transform;
    }

    void Camera2D::SetViewportSize(float width, float height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;

        float zoomX = width / m_TargetWidth;
        float zoomY = height / m_TargetHeight;
        m_BaseZoom = std::min(zoomX, zoomY);

        RecalculateViewMatrix();
    }

    glm::vec2 Camera2D::WorldToScreen(const glm::vec2 &worldPos) const
    {
        glm::vec4 screenPos = m_ViewMatrix * glm::vec4(worldPos, 0.0f, 1.0f);
        return glm::vec2(screenPos.x, screenPos.y);
    }

    glm::vec2 Camera2D::ScreenToWorld(const glm::vec2 &screenPos) const
    {
        glm::mat4 inverseView = glm::inverse(m_ViewMatrix);
        glm::vec4 worldPos = inverseView * glm::vec4(screenPos, 0.0f, 1.0f);
        return glm::vec2(worldPos.x, worldPos.y);
    }

    void Camera2D::GetVisibleWorldBounds(float &left, float &right, float &top, float &bottom) const
    {
        glm::vec2 bottomLeft = ScreenToWorld(glm::vec2(0, m_ViewportHeight));
        glm::vec2 topRight = ScreenToWorld(glm::vec2(m_ViewportWidth, 0));

        left = bottomLeft.x;
        right = topRight.x;
        top = topRight.y;
        bottom = bottomLeft.y;
    }
}
