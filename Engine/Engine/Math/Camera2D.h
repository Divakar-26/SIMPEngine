#pragma once
#include <Engine/Rendering/Renderer.h>
#include <Engine/Events/Event.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SIMPEngine
{
    class Camera2D
    {
    public:
        Camera2D(float zoom = 1.0f, const glm::vec2 &position = {0.0f, 0.0f});

        void SetPosition(const glm::vec2 &position);
        inline const glm::vec2 &GetPosition() const { return m_Position; }

        void Move(const glm::vec2 &delta);

        void SetZoom(float zoom);
        inline float GetZoom() const { return m_ManualZoom; }

        void SetRotation(float rotationDegrees);
        inline float GetRotation() const { return m_Rotation; }

        void OnEvent(Event &e);
        void Update(float deltaTime);

        glm::mat4 GetViewMatrix() const;

        void SetViewportSize(float width, float height);
        std::pair<int, int> GetViewportSize() { return {m_ViewportWidth, m_ViewportHeight}; }

        glm::vec2 WorldToScreen(const glm::vec2 &worldPos) const;

        glm::vec2 ScreenToWorld(const glm::vec2 &screenPos) const;

        void GetVisibleWorldBounds(float &left, float &right, float &top, float &bottom) const;

        glm::mat4 GetProjectionMatrix() const
        {
            float w = (float)m_ViewportWidth;
            float h = (float)m_ViewportHeight;
            float halfW = w * 0.5f / m_ManualZoom;
            float halfH = h * 0.5f / m_ManualZoom;

            return glm::ortho(-halfW, halfW, -halfH, halfH, -1.0f, 1.0f);
        }

        void SetCentered(bool centered) { isCentered = centered; }
        bool IsCentered() const { return isCentered; }

        void FocusOn(const glm::vec2 &position)
        {
            m_Position = position;
            m_TargetPosition = position;
            m_Dirty = true;
        }

        bool isCentered = false;

    private:
        glm::vec2 m_Position;
        float m_BaseZoom = 1.0f;   // zoom to fit scene
        float m_ManualZoom = 1.0f; // zoom from Q/E keys

        float m_Rotation;

        mutable bool m_Dirty;
        mutable glm::mat4 m_ViewMatrix;

        void RecalculateViewMatrix() const;

        glm::vec2 m_TargetPosition;
        float m_SmoothFactor;
        float m_MoveSpeed;
        float m_ZoomSpeed;

        bool m_MoveLeft = false;
        bool m_MoveRight = false;
        bool m_MoveUp = false;
        bool m_MoveDown = false;

        int m_ViewportWidth = 0;
        int m_ViewportHeight = 0;

        float m_TargetWidth = Renderer::m_WindowWidth;
        float m_TargetHeight = Renderer::m_WindowHeight;

        glm::vec2 m_LastMousePos{0.0f};
        bool m_Panning = false;
    };
}
