#include "Camera2D.h"
#include "Events/EventDispatcher.h"
#include "Events/KeyEvent.h"
#include <SDL3/SDL_keycode.h>

namespace SIMPEngine
{
    Camera2D::Camera2D(float zoom, const glm::vec2 &position) : 
        m_Position(position),
        m_TargetPosition(position),
        m_Zoom(zoom),
        m_Rotation(0.0f),
        m_SmoothFactor(10.0f),
        m_MoveSpeed(200.0f),
        m_ZoomSpeed(1.0f),
        m_Dirty(true),
        m_ViewMatrix(1.0f)
    {
    }

    void Camera2D::SetPosition(const glm::vec2 &position)
    {
        m_Position = position;
        m_Dirty = true;
    }

    void Camera2D::Move(const glm::vec2 &delta)
    {
        m_TargetPosition += delta; 
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

    void Camera2D::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent &ev)
                                             {
            HandleKeyPress(ev.GetKeyCode(), true);
            return false; });
        dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent &ev)
                                              {
            HandleKeyPress(ev.GetKeyCode(), false);
            return false; });
    }

    void Camera2D::HandleKeyPress(int key, bool pressed)
    {
        switch (key)
        {
        case SDLK_A:
            m_MoveLeft = pressed;
            break;
        case SDLK_D:
            m_MoveRight = pressed;
            break;
        case SDLK_W:
            m_MoveUp = pressed;
            break;
        case SDLK_S:
            m_MoveDown = pressed;
            break;
        case SDLK_Q:
            m_Zoom -= pressed ? m_ZoomSpeed * 0.1f : 0;
            break;
        case SDLK_E:
            m_Zoom += pressed ? m_ZoomSpeed * 0.1f : 0;
            break;
        default:
            break;
        }
    }

    void Camera2D::Update(float deltaTime)
    {
        m_Position += (m_TargetPosition - m_Position) * glm::clamp(m_SmoothFactor * deltaTime, 0.0f, 1.0f);

        glm::vec2 delta(0.0f);
        if (m_MoveLeft)
            delta.x -= m_MoveSpeed * deltaTime;
        if (m_MoveRight)
            delta.x += m_MoveSpeed * deltaTime;
        if (m_MoveUp)
            delta.y -= m_MoveSpeed * deltaTime;
        if (m_MoveDown)
            delta.y += m_MoveSpeed * deltaTime;

        Move(delta);
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
