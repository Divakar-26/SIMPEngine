#include "EditorLayer.h"
#include "imgui.h"

EditorLayer::EditorLayer(SIMPEngine::RenderingLayer* renderingLayer)
    : Layer("EditorLayer"), m_ViewportPanel(renderingLayer)
{
}

void EditorLayer::OnAttach()
{

}

void EditorLayer::OnDetach()
{

}

void EditorLayer::OnUpdate(class SIMPEngine::TimeStep ts)
{

}

void EditorLayer::OnRender()
{
    // Create Dockspace
    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Dockspace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::End();

    ImGui::Begin("Hierarchy");
    ImGui::Text("Entities will be listed here...");
    ImGui::End();

    ImGui::Begin("Inspector");
    ImGui::Text("Selected entity properties...");
    ImGui::End();

    m_ViewportPanel.OnRender();
}
