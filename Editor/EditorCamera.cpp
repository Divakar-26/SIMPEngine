#include "EditorCamera.h"

#include "UI/Theme.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

void EditorCamera::SetViewport(float width, float height)
{
    m_Width = std::max(1.0f, width);
    m_Height = std::max(1.0f, height);
}

glm::mat4 EditorCamera::GetView() const
{
    return glm::translate(glm::mat4(1.0f), glm::vec3(-m_Position, 0.0f));
}

glm::mat4 EditorCamera::GetProjection() const
{
    float hw = m_Width * 0.5f / m_Zoom;
    float hh = m_Height * 0.5f / m_Zoom;
    return glm::ortho(-hw, hw, -hh, hh, -100000.0f, 100000.0f);
}

glm::vec2 EditorCamera::ScreenToWorld(const glm::vec2 &pixel) const
{
    return {m_Position.x + (pixel.x - m_Width * 0.5f) / m_Zoom,
            m_Position.y - (pixel.y - m_Height * 0.5f) / m_Zoom};
}

glm::vec2 EditorCamera::WorldToScreen(const glm::vec2 &world) const
{
    return {(world.x - m_Position.x) * m_Zoom + m_Width * 0.5f,
            m_Height * 0.5f - (world.y - m_Position.y) * m_Zoom};
}

void EditorCamera::Pan(const glm::vec2 &pixelDelta)
{
    m_Position.x -= pixelDelta.x / m_Zoom;
    m_Position.y += pixelDelta.y / m_Zoom;
}

void EditorCamera::SetZoom(float zoom)
{
    m_Zoom = std::clamp(zoom, 0.02f, 64.0f);
}

void EditorCamera::ZoomAt(float steps, const glm::vec2 &pixel)
{
    glm::vec2 before = ScreenToWorld(pixel);
    SetZoom(m_Zoom * std::pow(EditorUI::Settings().ZoomStep, steps));
    glm::vec2 after = ScreenToWorld(pixel);
    m_Position += before - after;
}

void EditorCamera::Frame(const glm::vec2 &center, const glm::vec2 &size)
{
    m_Position = center;
    float fit = std::min(m_Width / std::max(size.x, 1.0f), m_Height / std::max(size.y, 1.0f));
    SetZoom(fit * 0.6f);
}
