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

ImVec2 mousePos;
ImVec2 windowPos;
ImVec2 contentMin;
ImVec2 contentMax;

ViewportPanel::ViewportPanel(SIMPEngine::RenderingLayer *renderingLayer)
    : m_RenderingLayer(renderingLayer)
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

void ViewportPanel::OnRender(SIMPEngine::Entity &m_SelectedEntity)
{
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

    // 🔹 Show OpenGL texture in ImGui
    ImTextureID texID = (ImTextureID)(intptr_t)api->GetViewportTexture();
    ImGui::Image(texID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));

    // Keep mouse/world position etc.
    mousePos = ImGui::GetMousePos();
    windowPos = ImGui::GetWindowPos();
    contentMin = ImGui::GetWindowContentRegionMin();
    contentMax = ImGui::GetWindowContentRegionMax();

    SelectEntites(m_SelectedEntity);
    RenderGizmos(m_SelectedEntity);

    DrawMouseWorldPosition();

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
    SIMPEngine::Renderer::DrawLine(-kEdge, 0.0f, kEdge, 0.0f, {255, 0, 0, 200}); // X axis
    SIMPEngine::Renderer::DrawLine(0.0f, -kEdge, 0.0f, kEdge, {0, 255, 0, 200}); // Y axis
}

void ViewportPanel::RenderGizmos(SIMPEngine::Entity &selectedEntity)
{
    if (!selectedEntity || !selectedEntity.HasComponent<TransformComponent>())
        return;

    auto &transform = selectedEntity.GetComponent<TransformComponent>();
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

    glm::mat4 entityMatrix = transform.GetTransform();

    static ImGuizmo::OPERATION currentOp = ImGuizmo::TRANSLATE;
    if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_M))
        currentOp = ImGuizmo::TRANSLATE;
    if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_N))
        currentOp = ImGuizmo::ROTATE;
    if (SIMPEngine::Input::IsKeyPressed(SIMPEngine::SIMPK_B))
        currentOp = ImGuizmo::SCALE;

    // draw gizmo
    ImGuizmo::Manipulate(
        glm::value_ptr(viewMatrix), glm::value_ptr(projectionMatrix),
        currentOp, ImGuizmo::WORLD, glm::value_ptr(entityMatrix));

    if (ImGuizmo::IsUsing())
    {
        glm::vec3 translation, rotation, scale, skew;
        glm::vec4 perspective;
        glm::quat orientation;

        glm::decompose(entityMatrix, scale, orientation, translation, skew, perspective);
        glm::vec3 euler = glm::eulerAngles(orientation);

        transform.position = {translation.x, translation.y};
        transform.rotation = glm::degrees(euler.z);
        transform.scale = {scale.x, scale.y};
    }
}

void ViewportPanel::SelectEntites(SIMPEngine::Entity &m_SelectedEntity)
{
    if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
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
            // CORE_INFO("Selected entity ID: {}");
            return;
        }
    }

    m_SelectedEntity = {};
}
