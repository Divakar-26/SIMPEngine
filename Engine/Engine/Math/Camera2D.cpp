#include <Engine/Math/Camera2D.h>
#include <Engine/Events/EventDispatcher.h>
#include <Engine/Events/KeyEvent.h>
#include <Engine/Core/Log.h>
#include <Engine/Input/Input.h>
#include <Engine/Input/SIMP_Keys.h>

#include <SDL3/SDL_keycode.h>

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
        m_Position +=
            (m_TargetPosition - m_Position) *
            glm::clamp(m_SmoothFactor * deltaTime, 0.0f, 1.0f);

        glm::vec2 moveDelta(0.0f);

        if (Input::IsKeyPressed(SIMPK_A))
            moveDelta.x -= m_MoveSpeed * deltaTime;

        if (Input::IsKeyPressed(SIMPK_D))
            moveDelta.x += m_MoveSpeed * deltaTime;

        if (Input::IsKeyPressed(SIMPK_W))
            moveDelta.y -= m_MoveSpeed * deltaTime;

        if (Input::IsKeyPressed(SIMPK_S))
            moveDelta.y += m_MoveSpeed * deltaTime;

        Move(moveDelta);

        int wheelDelta = Input::GetMouseWheel();

        if (wheelDelta > 0)
            m_ManualZoom += 0.1f * wheelDelta;

        if (wheelDelta < 0)
            m_ManualZoom -= 0.1f * -wheelDelta;

        m_ManualZoom =
            glm::clamp(m_ManualZoom, 0.1f, 10.0f);

        glm::vec2 mousePixel(
            Input::GetMousePosition().first,
            Input::GetMousePosition().second);

        if (Input::IsMouseButtonPressed(SIMPK_MOUSE_MIDDLE))
        {
            if (!m_Panning)
            {
                m_Panning = true;
                m_LastMousePixel = mousePixel;
            }
            else
            {
                glm::vec2 pixelDelta =
                    mousePixel - m_LastMousePixel;

                pixelDelta.y = -pixelDelta.y;

                float scale =
                    (540.0f * m_ManualZoom) /
                    (float)m_ViewportHeight;

                glm::vec2 worldDelta =
                    pixelDelta * scale;

                m_Position -= worldDelta;

                m_TargetPosition = m_Position;

                m_LastMousePixel = mousePixel;
            }
        }
        else
        {
            m_Panning = false;
        }

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
        glm::mat4 view(1.0f);

        // Move world opposite to camera
        view = glm::translate(view,
                              glm::vec3(-m_Position.x,
                                        -m_Position.y,
                                        0.0f));

        // Rotate world opposite to camera
        view = glm::rotate(view,
                           glm::radians(-m_Rotation),
                           glm::vec3(0, 0, 1));

        m_ViewMatrix = view;
        m_Dirty = false;
    }

    glm::mat4 Camera2D::GetProjectionMatrix() const
    {
        float aspect =
            (float)m_ViewportWidth /
            (float)m_ViewportHeight;

        float size = 540.0f * m_ManualZoom;

        return glm::ortho(
            -size * aspect,
            size * aspect,
            -size,
            size,
            -1.0f, 1.0f);
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

    glm::vec2 Camera2D::WorldToScreen(
        const glm::vec2 &worldPos) const
    {
        glm::mat4 VP =
            GetProjectionMatrix() * GetViewMatrix();

        glm::vec4 clip =
            VP * glm::vec4(worldPos, 0, 1);

        if (clip.w != 0)
            clip /= clip.w;

        float px =
            (clip.x + 1.0f) * 0.5f * m_ViewportWidth;

        float py =
            (1.0f - clip.y) * 0.5f * m_ViewportHeight;

        return {px, py};
    }

    glm::vec2 Camera2D::ScreenToWorld(
        const glm::vec2 &pixel) const
    {
        if (m_ViewportWidth == 0 || m_ViewportHeight == 0)
            return {0, 0};

        // pixel → NDC (-1 to 1)
        float ndcX =
            (2.0f * pixel.x) / m_ViewportWidth - 1.0f;

        float ndcY =
            1.0f - (2.0f * pixel.y) / m_ViewportHeight;

        glm::vec4 clip(ndcX, ndcY, 0, 1);

        glm::mat4 invVP =
            glm::inverse(
                GetProjectionMatrix() * GetViewMatrix());

        glm::vec4 world = invVP * clip;

        return {world.x, world.y};
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
