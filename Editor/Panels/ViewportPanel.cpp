#include "ViewportPanel.h"
#include "Core/Log.h"
#include "Input/Input.h"
#include "Rendering/Renderer.h"
#include "Input/SIMP_Keys.h"
#include "Core/VFS.h"
#include "Scene/Component.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <float.h>

ImVec2 mousePos;
ImVec2 windowPos;
ImVec2 contentMin;
ImVec2 contentMax;

constexpr float GIZMO_RADIUS = 8.0f;
constexpr ImU32 GIZMO_COLOR = IM_COL32(255, 255, 0, 255);
constexpr ImU32 GIZMO_HOVER_COLOR = IM_COL32(255, 0, 0, 255);

ViewportPanel::ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer)
    : m_RenderingLayer(renderingLayer)
{

    GizmoStyle style;
    style.Radius = 8.0f;
    style.HitRadius = 15.0f;
    style.NormalColor = IM_COL32(255, 255, 0, 255);
    style.HoverColor = IM_COL32(255, 0, 0, 255);
    style.ActiveColor = IM_COL32(0, 255, 0, 255);
    style.HoverRadiusMultiplier = 2.5f;
    m_GizmoSystem.SetStyle(style);
}

void ViewportPanel::OnAttach()
{
    auto &cam = m_RenderingLayer->GetCamera();
    int w = SIMPEngine::Renderer::m_WindowWidth;
    int h = SIMPEngine::Renderer::m_WindowHeight;
    cam.SetPosition({w / 2.0f, h / 2.0f});
    cam.SetZoom(0.60f);

    m_Context = &m_RenderingLayer->GetScene();
}

void ViewportPanel::OnRender(SIMPEngine::Entity &selectedEntity)
{
    m_SelectedEntity = selectedEntity;

    ImGui::Begin("Scene");

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    UpdateFocusState();

    ResizeViewportIfNeeded(viewportSize);

    // 🔹 Bind OpenGL framebuffer before rendering
    auto api = SIMPEngine::Renderer::GetAPI();
    api->BeginFrame();
    m_RenderingLayer->GetCamera().SetViewportSize(viewportSize.x, viewportSize.y);

    SIMPEngine::Renderer::Clear();
    RenderViewportBorder();
    OriginLines();
    m_RenderingLayer->OnRender();

    // 🔹 Unbind framebuffer
    api->EndFrame();

    ImTextureID texID = (ImTextureID)(intptr_t)api->GetViewportTexture();
    ImGui::Image(texID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

    mousePos = ImGui::GetMousePos();
    windowPos = ImGui::GetWindowPos();
    contentMin = ImGui::GetWindowContentRegionMin();
    contentMax = ImGui::GetWindowContentRegionMax();

    SelectEntities(selectedEntity);

    m_SelectedEntity = selectedEntity;

    RenderGizmos(m_SelectedEntity);

    DrawMouseWorldPosition();

    if (selectedEntity)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const char *srcVPath = (const char *)payload->Data;
                auto textureRealPath = SIMPEngine::VFS::Resolve(srcVPath);

                if (textureRealPath && m_SelectedEntity.HasComponent<SpriteComponent>())
                {
                    auto &sprite = m_SelectedEntity.GetComponent<SpriteComponent>();
                    sprite.texture = std::make_shared<SIMPEngine::Texture>();

                    if (sprite.texture->LoadFromFile(textureRealPath->c_str()))
                    {
                        if (m_SelectedEntity.HasComponent<RenderComponent>())
                        {
                            auto &render = m_SelectedEntity.GetComponent<RenderComponent>();
                            sprite.width = render.width;
                            sprite.height = render.height;
                        }
                        else
                        {
                            sprite.width = sprite.texture->GetWidth();
                            sprite.height = sprite.texture->GetHeight();
                        }

                        CORE_INFO("Updated sprite texture for entity: {}", srcVPath);
                    }
                    else
                    {
                        CORE_ERROR("Failed to load texture: {}", *textureRealPath);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::End();
}

void ViewportPanel::UpdateFocusState()
{
    bool nowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    bool nowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    SIMPEngine::Application::Get().GetImGuiLayer()->SetBlockEvent(!nowFocused);

    if (!nowFocused)
        SIMPEngine::Input::ResetAllKeys();

    m_ViewportFocused = nowFocused;
    m_ViewportHovered = nowHovered;
}

void ViewportPanel::ResizeViewportIfNeeded(const ImVec2 &viewportSize)
{
    static int lastW = -1, lastH = -1;
    int vw = (int)std::round(viewportSize.x);
    int vh = (int)std::round(viewportSize.y);

    if (vw != lastW || vh != lastH)
    {
        SIMPEngine::Renderer::GetAPI()->ResizeViewport(vw, vh);
        m_RenderingLayer->GetCamera().SetViewportSize((float)vw, (float)vh);
        lastW = vw;
        lastH = vh;
    }
}

void ViewportPanel::DrawMouseWorldPosition()
{
    glm::vec2 worldPos = m_RenderingLayer->GetCamera().ScreenToWorld(
        {mousePos.x - (windowPos.x + contentMin.x), mousePos.y - (windowPos.y + contentMin.y)});

    char posText[64];
    snprintf(posText, sizeof(posText), "X: %.2f Y: %.2f", worldPos.x, worldPos.y);

    ImVec2 textSize = ImGui::CalcTextSize(posText);
    ImVec2 textPos = {windowPos.x + contentMax.x - textSize.x - 10,
                      windowPos.y + contentMax.y - textSize.y - 10};

    ImGui::GetForegroundDrawList()->AddText(textPos, IM_COL32(255, 255, 255, 255), posText);
}

void ViewportPanel::RenderViewportBorder()
{
    SIMPEngine::Renderer::DrawQuad(0, 0, SIMPEngine::Renderer::m_WindowWidth, SIMPEngine::Renderer::m_WindowHeight, {255, 255, 255, 255}, false);
}

void ViewportPanel::OriginLines()
{
    constexpr float kEdge = 100000.0f;
    SIMPEngine::Renderer::DrawLine(-kEdge, 0.0f, kEdge, 0.0f, {255, 0, 0, 200});
    SIMPEngine::Renderer::DrawLine(0.0f, -kEdge, 0.0f, kEdge, {0, 255, 0, 200});
}

void ViewportPanel::RenderGizmos(SIMPEngine::Entity &selectedEntity)
{
    if (!selectedEntity || !selectedEntity.HasComponent<TransformComponent>())
        return;

    auto &transform = selectedEntity.GetComponent<TransformComponent>();

    glm::vec2 corners[4];
    bool hasRender = selectedEntity.HasComponent<RenderComponent>();
    bool hasCollider = selectedEntity.HasComponent<CollisionComponent>();

    if (hasRender)
    {
        auto &render = selectedEntity.GetComponent<RenderComponent>();
        CalculateEntityCorners(transform, render, corners);
    }
    else if (hasCollider)
    {
        auto &collider = selectedEntity.GetComponent<CollisionComponent>();
        float x = transform.position.x + collider.offsetX;
        float y = transform.position.y + collider.offsetY;
        float w = collider.width;
        float h = collider.height;

        corners[0] = {x, y};
        corners[1] = {x + w, y};
        corners[2] = {x, y + h};
        corners[3] = {x + w, y + h};
    }
    else
    {
        return; // Nothing to draw gizmo for
    }

    auto worldToScreen = [this](const glm::vec2 &worldPos) -> glm::vec2
    {
        return this->WorldToScreen(worldPos);
    };

    GizmoHandle hoveredGizmo = m_GizmoSystem.DrawHandles(corners, worldToScreen);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        if (hoveredGizmo != GizmoHandle::None)
        {
            m_GizmoSystem.OnMouseClick(true);
            if (m_GizmoSystem.IsInteracting())
            {
                m_DragStartWorldPos = GetMouseWorldPos();
                m_DragStartEntityPos = transform.position;
                m_DragStartEntityScale = transform.scale;

                if (selectedEntity.HasComponent<CollisionComponent>())
                {
                    auto &collider = selectedEntity.GetComponent<CollisionComponent>();
                    m_DragStartColliderWidth = collider.width;
                    m_DragStartColliderHeight = collider.height;
                    m_DragStartColliderOffsetX = collider.offsetX;
                    m_DragStartColliderOffsetY = collider.offsetY;
                }

                CORE_INFO("Started dragging gizmo: {}", (int)hoveredGizmo);
            }
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (m_GizmoSystem.IsDragging())
        {
            CORE_INFO("Stopped dragging gizmo");
        }
        m_GizmoSystem.OnMouseRelease();
    }

    if (m_GizmoSystem.IsDragging())
    {
        glm::vec2 mouseWorldPos = GetMouseWorldPos();
        glm::vec2 delta = mouseWorldPos - m_DragStartWorldPos;

        glm::vec2 baseSize(1.0f);
        if (hasRender)
        {
            auto &render = selectedEntity.GetComponent<RenderComponent>();
            baseSize = {render.width, render.height};
        }
        else if (hasCollider)
        {
            auto &collider = selectedEntity.GetComponent<CollisionComponent>();
            baseSize = {collider.width, collider.height};
        }

        auto gizmoDelta = m_GizmoSystem.CalculateTransformDelta(
            m_GizmoSystem.GetActiveGizmo(),
            delta,
            baseSize);

        transform.position = m_DragStartEntityPos + gizmoDelta.positionDelta;
        transform.scale = m_DragStartEntityScale + gizmoDelta.scaleDelta;

        transform.scale.x = glm::max(transform.scale.x, 0.01f);
        transform.scale.y = glm::max(transform.scale.y, 0.01f);

        if (selectedEntity.HasComponent<CollisionComponent>() && hasCollider)
        {
            auto &collider = selectedEntity.GetComponent<CollisionComponent>();

            collider.width = glm::max(1.0f, m_DragStartColliderWidth + gizmoDelta.scaleDelta.x);
            collider.height = glm::max(1.0f, m_DragStartColliderHeight + gizmoDelta.scaleDelta.y);

            collider.offsetX = m_DragStartColliderOffsetX + gizmoDelta.positionDelta.x;
            collider.offsetY = m_DragStartColliderOffsetY + gizmoDelta.positionDelta.y;
        }
    }

    if (!m_GizmoSystem.IsInteracting())
    {
        RenderImGuizmo(selectedEntity, transform);
    }
}

void ViewportPanel::RenderImGuizmo(SIMPEngine::Entity &selectedEntity, TransformComponent &transform)
{
    if (!selectedEntity.HasComponent<RenderComponent>())
        return;

    auto &render = selectedEntity.GetComponent<RenderComponent>();
    auto &cam = m_RenderingLayer->GetCamera();

    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist();

    ImVec2 rectMin = {windowPos.x + contentMin.x, windowPos.y + contentMin.y};
    ImVec2 rectMax = {windowPos.x + contentMax.x, windowPos.y + contentMax.y};

    ImGuizmo::SetRect(rectMin.x, rectMin.y, rectMax.x - rectMin.x, rectMax.y - rectMin.y);

    glm::mat4 viewMatrix = cam.GetViewMatrix();
    glm::mat4 projectionMatrix = glm::ortho(
        0.0f, (float)cam.GetViewportSize().first,
        (float)cam.GetViewportSize().second, 0.0f,
        -1.0f, 1.0f);

    static ImGuizmo::OPERATION currentOp = ImGuizmo::TRANSLATE;
    if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_M))
        currentOp = ImGuizmo::TRANSLATE;
    if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_N))
        currentOp = ImGuizmo::ROTATE;
    if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_B))
        currentOp = ImGuizmo::SCALE;

    glm::vec2 center = transform.position + glm::vec2(
                                                (render.width * transform.scale.x) / 2.0f,
                                                (render.height * transform.scale.y) / 2.0f);

    glm::mat4 centeredMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f)) *
                               glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation), glm::vec3(0.0f, 0.0f, 1.0f)) *
                               glm::scale(glm::mat4(1.0f), glm::vec3(transform.scale, 1.0f));

    ImGuizmo::Manipulate(
        glm::value_ptr(viewMatrix), glm::value_ptr(projectionMatrix),
        currentOp, ImGuizmo::WORLD, glm::value_ptr(centeredMatrix));

    if (ImGuizmo::IsUsing())
    {
        glm::vec3 translation, rotation, scale, skew;
        glm::vec4 perspective;
        glm::quat orientation;

        glm::decompose(centeredMatrix, scale, orientation, translation, skew, perspective);
        glm::vec3 euler = glm::eulerAngles(orientation);

        transform.position = {translation.x - (render.width * scale.x) / 2.0f,
                              translation.y - (render.height * scale.y) / 2.0f};
        transform.rotation = glm::degrees(euler.z);
        transform.scale = {scale.x, scale.y};
    }
}

void ViewportPanel::CalculateEntityCorners(const TransformComponent &transform,
                                           const RenderComponent &render,
                                           glm::vec2 corners[4])
{
    float width = render.width * transform.scale.x;
    float height = render.height * transform.scale.y;

    corners[0] = transform.position;
    corners[1] = transform.position + glm::vec2(width, 0);
    corners[2] = transform.position + glm::vec2(0, height);
    corners[3] = transform.position + glm::vec2(width, height);
}

glm::vec2 ViewportPanel::WorldToScreen(const glm::vec2 &worldPos)
{
    glm::vec2 screenPos = m_RenderingLayer->GetCamera().WorldToScreen(worldPos);
    screenPos.x += windowPos.x + contentMin.x;
    screenPos.y += windowPos.y + contentMin.y;
    return screenPos;
}

void ViewportPanel::SelectEntities(SIMPEngine::Entity &selectedEntity)
{
    if (IsClickingOnGizmo())
        return;
    if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
        return;
    if (!m_ViewportHovered || !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    glm::vec2 localMouse = {
        mousePos.x - (windowPos.x + contentMin.x),
        mousePos.y - (windowPos.y + contentMin.y)};
    glm::vec2 worldMouse = m_RenderingLayer->GetCamera().ScreenToWorld(localMouse);

    SIMPEngine::Entity topEntity;
    SIMPEngine::Entity topCollision;
    float topZIndex = -FLT_MAX;

    // Check sprites first
    auto view = m_Context->GetRegistry().view<TransformComponent, RenderComponent>();
    for (auto entity : view)
    {
        auto &transform = view.get<TransformComponent>(entity);
        auto &render = view.get<RenderComponent>(entity);

        float left = transform.position.x;
        float right = transform.position.x + render.width * transform.scale.x;
        float top = transform.position.y;
        float bottom = transform.position.y + render.height * transform.scale.y;

        if (worldMouse.x >= left && worldMouse.x <= right &&
            worldMouse.y >= top && worldMouse.y <= bottom)
        {
            if (transform.zIndex > topZIndex)
            {
                topZIndex = transform.zIndex;
                topEntity = SIMPEngine::Entity{entity, m_Context};
            }
        }
    }

    // Check collisions
    auto collisionView = m_Context->GetRegistry().view<TransformComponent, CollisionComponent>();
    for (auto entity : collisionView)
    {
        auto &transform = collisionView.get<TransformComponent>(entity);
        auto &collider = collisionView.get<CollisionComponent>(entity);

        float left = transform.position.x;
        float right = transform.position.x + collider.width * transform.scale.x;
        float top = transform.position.y;
        float bottom = transform.position.y + collider.height * transform.scale.y;

        if (worldMouse.x >= left && worldMouse.x <= right &&
            worldMouse.y >= top && worldMouse.y <= bottom)
        {
            if (transform.zIndex > topZIndex)
            {
                topZIndex = transform.zIndex;
                topCollision = SIMPEngine::Entity{entity, m_Context};
            }
        }
    }

    // Decide what to select
    if (topCollision)
    {
        m_SelectedEntity = topCollision;
        selectedEntity = topCollision;

        if (topCollision.HasComponent<CollisionComponent>())
            LOG_INFO("Selected Collider: Entity ID {}", (int)topEntity.GetHandle());
        else if (topCollision.HasComponent<RenderComponent>())
            LOG_INFO("Selected Sprite: Entity ID {}", (int)topEntity.GetHandle());
    }
    else if (topEntity)
    {
        m_SelectedEntity = topEntity;
        selectedEntity = topEntity;
    }
    else
    {
        m_SelectedEntity = {};
        selectedEntity = {};
    }
}

glm::vec2 ViewportPanel::GetMouseWorldPos() const
{
    return m_RenderingLayer->GetCamera().ScreenToWorld({mousePos.x - (windowPos.x + contentMin.x),
                                                        mousePos.y - (windowPos.y + contentMin.y)});
}

bool ViewportPanel::IsClickingOnGizmo()
{
    if (!m_ViewportHovered || !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return false;

    return m_GizmoSystem.IsInteracting();
}