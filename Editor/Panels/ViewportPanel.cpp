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
    : m_RenderingLayer(renderingLayer), m_ActiveGizmo(GizmoHandle::None), m_IsDraggingGizmo(false)
{
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

    SelectEntites(selectedEntity);
    
    m_SelectedEntity = selectedEntity;
    
    RenderGizmos(m_SelectedEntity);

    DrawMouseWorldPosition();

    if(selectedEntity){
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
    if (!selectedEntity || !selectedEntity.HasComponent<TransformComponent>() ||
        !selectedEntity.HasComponent<RenderComponent>())
        return;

    auto &transform = selectedEntity.GetComponent<TransformComponent>();
    auto &render = selectedEntity.GetComponent<RenderComponent>();

    if (render.width == 0.0f && render.height == 0.0f)
    {
        RenderImGuizmo(selectedEntity, transform);
        return;
    }

    glm::vec2 corners[4];
    CalculateEntityCorners(transform, render, corners);

    GizmoHandle hoveredGizmo = DrawGizmoHandles(corners);

    HandleGizmoDragging(selectedEntity, transform, render, corners, hoveredGizmo);

    if (!m_IsDraggingGizmo)
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

GizmoHandle ViewportPanel::DrawGizmoHandles(const glm::vec2 corners[4])
{
    GizmoHandle hoveredGizmo = GizmoHandle::None;
    float minDist = FLT_MAX;

    DrawSingleGizmo(corners[0], GizmoHandle::TopLeft, hoveredGizmo, minDist);
    DrawSingleGizmo(corners[1], GizmoHandle::TopRight, hoveredGizmo, minDist);
    DrawSingleGizmo(corners[2], GizmoHandle::BottomLeft, hoveredGizmo, minDist);
    DrawSingleGizmo(corners[3], GizmoHandle::BottomRight, hoveredGizmo, minDist);

    glm::vec2 topCenter = (corners[0] + corners[1]) * 0.5f;
    glm::vec2 bottomCenter = (corners[2] + corners[3]) * 0.5f;
    glm::vec2 leftCenter = (corners[0] + corners[2]) * 0.5f;
    glm::vec2 rightCenter = (corners[1] + corners[3]) * 0.5f;

    DrawSingleGizmo(topCenter, GizmoHandle::TopCenter, hoveredGizmo, minDist);
    DrawSingleGizmo(bottomCenter, GizmoHandle::BottomCenter, hoveredGizmo, minDist);
    DrawSingleGizmo(leftCenter, GizmoHandle::LeftCenter, hoveredGizmo, minDist);
    DrawSingleGizmo(rightCenter, GizmoHandle::RightCenter, hoveredGizmo, minDist);

    return hoveredGizmo;
}

void ViewportPanel::DrawSingleGizmo(const glm::vec2 &worldPos, GizmoHandle handleType, 
                                  GizmoHandle &hoveredGizmo, float &minDist)
{
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    
    glm::vec2 screenPos = WorldToScreen(worldPos);
    
    // Check if the gizmo is within the viewport bounds
    ImVec2 viewportMin = {windowPos.x + contentMin.x, windowPos.y + contentMin.y};
    ImVec2 viewportMax = {windowPos.x + contentMax.x, windowPos.y + contentMax.y};
    
    if (screenPos.x < viewportMin.x || screenPos.x > viewportMax.x ||
        screenPos.y < viewportMin.y || screenPos.y > viewportMax.y)
    {
        return; // Skip drawing if outside viewport
    }
    
    glm::vec2 mouseScreen = { mousePos.x, mousePos.y };
    float dist = glm::distance(mouseScreen, screenPos);
    
    bool isHovered = (dist <= GIZMO_RADIUS * 1.8f);
    bool isActive = (m_ActiveGizmo == handleType);
    
    ImU32 color = isActive ? GIZMO_HOVER_COLOR : (isHovered ? GIZMO_HOVER_COLOR : GIZMO_COLOR);
    
    drawList->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), GIZMO_RADIUS, color);
    drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), GIZMO_RADIUS, IM_COL32(0, 0, 0, 255), 12, 1.0f);
    
    if (isHovered)
    {
        drawList->AddCircle(ImVec2(screenPos.x, screenPos.y), GIZMO_RADIUS * 1.8f, 
                           IM_COL32(255, 255, 255, 100), 12, 2.0f);
    }
    
    if (isHovered && dist < minDist)
    {
        hoveredGizmo = handleType;
        minDist = dist;
    }
}

void ViewportPanel::HandleGizmoDragging(SIMPEngine::Entity &entity,
                                        TransformComponent &transform,
                                        RenderComponent &render,
                                        const glm::vec2 corners[4],
                                        GizmoHandle hoveredGizmo)
{
    glm::vec2 mouseWorldPos = m_RenderingLayer->GetCamera().ScreenToWorld(
        {mousePos.x - (windowPos.x + contentMin.x), mousePos.y - (windowPos.y + contentMin.y)});

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hoveredGizmo != GizmoHandle::None)
    {
        m_ActiveGizmo = hoveredGizmo;
        m_IsDraggingGizmo = true;
        m_DragStartWorldPos = mouseWorldPos;
        m_DragStartEntityPos = transform.position;
        m_DragStartEntityScale = transform.scale;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        m_ActiveGizmo = GizmoHandle::None;
        m_IsDraggingGizmo = false;
    }

    if (m_IsDraggingGizmo && m_ActiveGizmo != GizmoHandle::None)
    {
        glm::vec2 delta = mouseWorldPos - m_DragStartWorldPos;

        switch (m_ActiveGizmo)
        {
        case GizmoHandle::TopLeft:
            transform.position.x = m_DragStartEntityPos.x + delta.x;
            transform.position.y = m_DragStartEntityPos.y + delta.y;
            transform.scale.x = m_DragStartEntityScale.x - delta.x / render.width;
            transform.scale.y = m_DragStartEntityScale.y - delta.y / render.height;
            break;

        case GizmoHandle::TopRight:
            transform.position.y = m_DragStartEntityPos.y + delta.y;
            transform.scale.x = m_DragStartEntityScale.x + delta.x / render.width;
            transform.scale.y = m_DragStartEntityScale.y - delta.y / render.height;
            break;

        case GizmoHandle::BottomLeft:
            transform.position.x = m_DragStartEntityPos.x + delta.x;
            transform.scale.x = m_DragStartEntityScale.x - delta.x / render.width;
            transform.scale.y = m_DragStartEntityScale.y + delta.y / render.height;
            break;

        case GizmoHandle::BottomRight:
            transform.scale.x = m_DragStartEntityScale.x + delta.x / render.width;
            transform.scale.y = m_DragStartEntityScale.y + delta.y / render.height;
            break;

        case GizmoHandle::TopCenter:
            transform.position.y = m_DragStartEntityPos.y + delta.y;
            transform.scale.y = m_DragStartEntityScale.y - delta.y / render.height;
            break;

        case GizmoHandle::BottomCenter:
            transform.scale.y = m_DragStartEntityScale.y + delta.y / render.height;
            break;

        case GizmoHandle::LeftCenter:
            transform.position.x = m_DragStartEntityPos.x + delta.x;
            transform.scale.x = m_DragStartEntityScale.x - delta.x / render.width;
            break;

        case GizmoHandle::RightCenter:
            transform.scale.x = m_DragStartEntityScale.x + delta.x / render.width;
            break;

        default:
            break;
        }

        transform.scale.x = glm::max(transform.scale.x, 0.01f);
        transform.scale.y = glm::max(transform.scale.y, 0.01f);
    }
}

glm::vec2 ViewportPanel::WorldToScreen(const glm::vec2 &worldPos)
{
    glm::vec2 screenPos = m_RenderingLayer->GetCamera().WorldToScreen(worldPos);
    screenPos.x += windowPos.x + contentMin.x;
    screenPos.y += windowPos.y + contentMin.y;
    return screenPos;
}

void ViewportPanel::SelectEntites(SIMPEngine::Entity &selectedEntity)
{
    if (IsClickingOnGizmo())
        return;

    if (ImGuizmo::IsOver() || ImGuizmo::IsUsing() || m_IsDraggingGizmo)
        return;

    if (!m_ViewportHovered || !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    glm::vec2 localMouse = {
        mousePos.x - (windowPos.x + contentMin.x),
        mousePos.y - (windowPos.y + contentMin.y)};

    glm::vec2 worldMouse = m_RenderingLayer->GetCamera().ScreenToWorld(localMouse);

    auto view = m_Context->GetRegistry().view<TransformComponent, RenderComponent>();
    for (auto entity : view)
    {
        auto &transform = view.get<TransformComponent>(entity);
        auto &sprite = view.get<RenderComponent>(entity);

        float left = transform.position.x;
        float right = transform.position.x + sprite.width * transform.scale.x;
        float top = transform.position.y;
        float bottom = transform.position.y + sprite.height * transform.scale.y;

        if (worldMouse.x >= left && worldMouse.x <= right &&
            worldMouse.y >= top && worldMouse.y <= bottom)
        {
            m_SelectedEntity = SIMPEngine::Entity{entity, m_Context};
            selectedEntity = m_SelectedEntity; 
            return;
        }
    }

    m_SelectedEntity = {};
    selectedEntity = {}; 
}

bool ViewportPanel::IsClickingOnGizmo()
{
    if (!m_ViewportHovered || !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return false;

    if (!m_SelectedEntity || !m_SelectedEntity.HasComponent<TransformComponent>() ||
        !m_SelectedEntity.HasComponent<RenderComponent>())
        return false;

    auto &transform = m_SelectedEntity.GetComponent<TransformComponent>();
    auto &render = m_SelectedEntity.GetComponent<RenderComponent>();

    glm::vec2 corners[4];
    CalculateEntityCorners(transform, render, corners);

    glm::vec2 localMouse = {
        mousePos.x - (windowPos.x + contentMin.x),
        mousePos.y - (windowPos.y + contentMin.y)};

    if (localMouse.x < 0 || localMouse.y < 0 || 
        localMouse.x > (contentMax.x - contentMin.x) || 
        localMouse.y > (contentMax.y - contentMin.y))
    {
        return false; 
    }

    glm::vec2 worldMouse = m_RenderingLayer->GetCamera().ScreenToWorld(localMouse);

    for (int i = 0; i < 4; i++)
    {
        glm::vec2 screenCorner = WorldToScreen(corners[i]);
        if (screenCorner.x >= (windowPos.x + contentMin.x) && 
            screenCorner.x <= (windowPos.x + contentMax.x) &&
            screenCorner.y >= (windowPos.y + contentMin.y) && 
            screenCorner.y <= (windowPos.y + contentMax.y))
        {
            if (glm::distance(worldMouse, corners[i]) <= GIZMO_RADIUS * 2.0f) 
                return true;
        }
    }

    glm::vec2 topCenter = (corners[0] + corners[1]) * 0.5f;
    glm::vec2 bottomCenter = (corners[2] + corners[3]) * 0.5f;
    glm::vec2 leftCenter = (corners[0] + corners[2]) * 0.5f;
    glm::vec2 rightCenter = (corners[1] + corners[3]) * 0.5f;

    glm::vec2 edgeCenters[] = {topCenter, bottomCenter, leftCenter, rightCenter};
    for (const auto &center : edgeCenters)
    {
        glm::vec2 screenCenter = WorldToScreen(center);
        if (screenCenter.x >= (windowPos.x + contentMin.x) && 
            screenCenter.x <= (windowPos.x + contentMax.x) &&
            screenCenter.y >= (windowPos.y + contentMin.y) && 
            screenCenter.y <= (windowPos.y + contentMax.y))
        {
            if (glm::distance(worldMouse, center) <= GIZMO_RADIUS * 2.0f)
                return true;
        }
    }

    return false;
}