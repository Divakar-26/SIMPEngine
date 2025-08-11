#pragma once

#include<glm/glm.hpp>

namespace SIMPEngine
{
    class Camera2D{
        public:
            Camera2D();

            void SetPosition(const glm::vec2& position);
            const glm::vec2 & GetPosition() const;

            void Move(const glm::vec2 & delta);

            void SetZoom(float zoom);
            float GetZoom() const;

            glm::mat4 GetViewMatrix() const;

        private:
        glm::vec2 m_Position;
        float m_Zoom;

        mutable bool m_ViewMatrixNeedsUpdate;
        mutable glm::mat4 m_ViewMatrix;

        void RecalculateViewMatrix() const;

    };
} // namespace SIMPEngine
